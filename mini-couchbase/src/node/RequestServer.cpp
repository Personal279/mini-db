// node/RequestServer.cpp
#include "node/RequestServer.h"

#include <functional>
#include <iostream>

RequestServer::RequestServer(
    PartitionManager& partitionManager,
    int partitionCount
)
    : partitionManager(partitionManager),
      partitionCount(partitionCount) {
}

int RequestServer::findPartition(const std::string& key) const {
    std::size_t hashValue = std::hash<std::string>{}(key);
    return static_cast<int>(hashValue % partitionCount);
}

Response RequestServer::handleRequest(const Request& request) {
    int partitionId = findPartition(request.key);

    if (!partitionManager.ownsPartition(partitionId)) {
        std::cout << "This node does not own partition " << partitionId << std::endl;
        return { false, false, "", "Does not own partition" };
    }

    std::cout << "Request accepted for partition " << partitionId << std::endl;

    return partitionManager.processRequest(request);
}