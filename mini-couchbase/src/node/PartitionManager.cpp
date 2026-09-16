// node/PartitionManager.cpp
#include "node/PartitionManager.h"

#include <iostream>
#include <sstream>

PartitionManager::PartitionManager(
    StorageEngine& storageEngine,
    RaftGroup& raftGroup
)
    : storageEngine(storageEngine),
      raftGroup(raftGroup) {
}

void PartitionManager::addPartition(int partitionId) {
    partitions.push_back(partitionId);
}

bool PartitionManager::ownsPartition(int partitionId) const {
    for (int id : partitions) {
        if (id == partitionId) {
            return true;
        }
    }
    return false;
}

Response PartitionManager::processRequest(const Request& request) {
    switch (request.operation) {

        case Operation::PUT:
        case Operation::DELETE:

            if (raftGroup.getRole() != RaftRole::LEADER) {
                std::cout << "Request rejected: this node is not the Raft leader" << std::endl;
                return { false, false, "", "Not leader" };
            }

            appendWriteCommand(request);
            return { true, true, "", "Accepted" };

        case Operation::GET: {
            std::string value = storageEngine.get(request.key);
            std::cout << "GET result: " << value << std::endl;
            return { true, true, value, "OK" };
        }
    }

    return { false, false, "", "Unknown operation" };
}

void PartitionManager::appendWriteCommand(const Request& request) {
    std::string operation;

    if (request.operation == Operation::PUT) {
        operation = "PUT";
    } else {
        operation = "DELETE";
    }

    std::string command = operation + "|" + request.key + "|" + request.value;

    int index = raftGroup.getLog().getLastIndex() + 1;
    int term = raftGroup.getCurrentTerm();

    LogEntry entry{ term, index, command };

    raftGroup.getLog().append(entry);

    std::cout << "Raft log appended: " << command
               << " [term=" << term << ", index=" << index << "]" << std::endl;
}

void PartitionManager::applyCommand(const std::string& command) {
    std::stringstream stream(command);

    std::string operation;
    std::string key;
    std::string value;

    std::getline(stream, operation, '|');
    std::getline(stream, key, '|');
    std::getline(stream, value);

    Request request;

    if (operation == "PUT") {
        request.operation = Operation::PUT;
    } else if (operation == "DELETE") {
        request.operation = Operation::DELETE;
    } else {
        return;
    }

    request.key = key;
    request.value = value;

    switch (request.operation) {

        case Operation::PUT:
            storageEngine.put(request.key, request.value);
            std::cout << "Committed PUT applied: " << request.key << std::endl;
            break;

        case Operation::DELETE:
            storageEngine.remove(request.key);
            std::cout << "Committed DELETE applied: " << request.key << std::endl;
            break;

        case Operation::GET:
            break;
    }
}