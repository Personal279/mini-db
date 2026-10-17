
#include "node/PartitionManager.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

PartitionManager::PartitionManager(
    StorageEngine& storageEngine,
    const PartitionRouter& partitionRouter
)
    : storageEngine(storageEngine),
      partitionRouter(partitionRouter),
      indexClient() {
}

void PartitionManager::addPartition(
    int partitionId,
    RaftGroup& raftGroup
) {
    if (raftGroup.getPartitionId() != partitionId) {
        throw std::invalid_argument(
            "Raft group partition ID does not match registration"
        );
    }

    auto [it, inserted] = raftGroups.emplace(
        partitionId,
        &raftGroup
    );

    if (!inserted) {
        throw std::invalid_argument(
            "Partition already registered"
        );
    }
}

bool PartitionManager::ownsPartition(int partitionId) const {
    return raftGroups.find(partitionId) != raftGroups.end();
}

RaftGroup& PartitionManager::getRaftGroup(int partitionId) {
    auto it = raftGroups.find(partitionId);

    if (it == raftGroups.end()) {
        throw std::out_of_range(
            "No local Raft group for partition"
        );
    }

    return *it->second;
}

int PartitionManager::findPartition(
    const std::string& key
) const {
    return partitionRouter.findPartition(key);
}

Response PartitionManager::processRequest(
    const Request& request
) {
    const int partitionId = findPartition(request.key);

    if (!ownsPartition(partitionId)) {
        return {
            false, false, "",
            "This node is not a replica of the partition"
        };
    }

    RaftGroup& raftGroup = getRaftGroup(partitionId);

    switch (request.operation) {
        case Operation::PUT:
        case Operation::DELETE: {
            if (raftGroup.getRole() != RaftRole::LEADER) {
                return {false, false, "", "Not leader"};
            }

            const int index = appendWriteCommand(
                raftGroup,
                request
            );

            const auto deadline =
                std::chrono::steady_clock::now()
                + std::chrono::seconds(2);

            while (raftGroup.getLastApplied() < index) {
                if (raftGroup.getRole() != RaftRole::LEADER) {
                    return {
                        false, false, "",
                        "Leadership lost before commit"
                    };
                }

                if (std::chrono::steady_clock::now() >= deadline) {
                    return {false, false, "", "Commit timeout"};
                }

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(5)
                );
            }

            return {true, true, "", "Accepted"};
        }

        case Operation::GET: {
            if (raftGroup.getRole() != RaftRole::LEADER) {
                return {false, false, "", "Not leader"};
            }

            const std::string value =
                storageEngine.get(request.key);

            return {true, true, value, "OK"};
        }
    }

    return {false, false, "", "Unknown operation"};
}

int PartitionManager::appendWriteCommand(
    RaftGroup& raftGroup,
    const Request& request
) {
    const std::string operation =
        request.operation == Operation::PUT
        ? "PUT"
        : "DELETE";

    const std::string command =
        operation + "|" + request.key + "|" + request.value;

    const int index = raftGroup.getLog().getLastIndex() + 1;
    const int term = raftGroup.getCurrentTerm();

    LogEntry entry{term, index, command};
    raftGroup.getLog().append(entry);

    std::cout << "Raft log appended for partition "
              << raftGroup.getPartitionId()
              << ": " << command
              << " [term=" << term
              << ", index=" << index << "]"
              << std::endl;

    return index;
}

void PartitionManager::applyCommand(
    int partitionId,
    const std::string& command
) {
    // Reject commands for partitions not hosted by this node.
    if (!ownsPartition(partitionId)) {
        throw std::out_of_range(
            "Cannot apply command for unowned partition"
        );
    }

    std::stringstream stream(command);

    std::string operation;
    std::string key;
    std::string value;

    std::getline(stream, operation, '|');
    std::getline(stream, key, '|');
    std::getline(stream, value);

    if (operation == "PUT") {
        storageEngine.put(key, value);

        try {
            indexClient.upsertDocument(key, value);
        } catch (const std::exception& e) {
            std::cerr << "Index synchronization failed: "
                      << e.what() << std::endl;
        }
    } else if (operation == "DELETE") {
        storageEngine.remove(key);

        try {
            indexClient.deleteDocument(key);
        } catch (const std::exception& e) {
            std::cerr << "Index synchronization failed: "
                      << e.what() << std::endl;
        }
    }
}


QueryResult PartitionManager::executeQuery(
    const QueryPredicate& predicate
) {
    QueryExecutor executor(storageEngine, indexClient);

    // Temporary estimate until we add storage statistics.
    constexpr std::size_t estimatedDocumentCount = 1000;

    return executor.execute(
        predicate,
        estimatedDocumentCount,
        0.10
    );
}