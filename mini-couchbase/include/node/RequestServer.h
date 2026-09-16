// node/RequestServer.h
#pragma once

#include "common/Types.h"
#include "node/PartitionManager.h"

class RequestServer {
public:
    RequestServer(PartitionManager& partitionManager, int partitionCount);

    Response handleRequest(const Request& request);

private:
    PartitionManager& partitionManager;
    int partitionCount;

    int findPartition(const std::string& key) const;
};