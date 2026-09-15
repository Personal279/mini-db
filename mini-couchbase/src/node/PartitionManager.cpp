#include "node/PartitionManager.h"

#include <iostream>
#include <sstream>

#include "raft/RaftGroup.h"

PartitionManager::PartitionManager(
    StorageEngine& storageEngine
)
    : storageEngine(storageEngine),
      raftGroup(nullptr) {
}

void PartitionManager::setRaftGroup(
    RaftGroup& raftGroup
) {
    this->raftGroup = &raftGroup;
}

void PartitionManager::addPartition(
    int partitionId
) {
    partitions.push_back(partitionId);
}

bool PartitionManager::ownsPartition(
    int partitionId
) const {
    for (int id : partitions) {
        if (id == partitionId) {
            return true;
        }
    }

    return false;
}

void PartitionManager::processRequest(
    const Request& request
) {
    if (request.operation == Operation::GET) {
        std::string value =
            storageEngine.get(request.key);

        std::cout
            << "GET result: "
            << value
            << std::endl;

        return;
    }

    if (raftGroup == nullptr) {
        std::cout
            << "Raft group not configured"
            << std::endl;

        return;
    }

    if (
        raftGroup->getRole() !=
        RaftRole::LEADER
    ) {
        std::cout
            << "Request rejected: node is not leader"
            << std::endl;

        return;
    }

    std::string command;

    if (request.operation == Operation::PUT) {
        command =
            "PUT|" +
            request.key +
            "|" +
            request.value;
    }
    else if (request.operation == Operation::DELETE) {
        command =
            "DELETE|" +
            request.key;
    }
    else {
        return;
    }

    raftGroup->getLog().append(
        raftGroup->getCurrentTerm(),
        command
    );

    std::cout
        << "Command appended to Raft log: "
        << command
        << std::endl;
}

void PartitionManager::applyCommand(
    const std::string& command
) {
    std::stringstream stream(command);

    std::string operation;
    std::string key;
    std::string value;

    std::getline(
        stream,
        operation,
        '|'
    );

    std::getline(
        stream,
        key,
        '|'
    );

    std::getline(
        stream,
        value
    );

    Request request;

    if (operation == "PUT") {
        request.operation = Operation::PUT;
    }
    else if (operation == "GET") {
        request.operation = Operation::GET;
    }
    else if (operation == "DELETE") {
        request.operation = Operation::DELETE;
    }
    else {
        return;
    }

    request.key = key;
    request.value = value;

    switch (request.operation) {

        case Operation::PUT:
            storageEngine.put(
                request.key,
                request.value
            );

            std::cout
                << "Applied PUT: "
                << request.key
                << std::endl;
            break;

        case Operation::GET:
            break;

        case Operation::DELETE:
            storageEngine.remove(
                request.key
            );

            std::cout
                << "Applied DELETE: "
                << request.key
                << std::endl;
            break;
    }
}