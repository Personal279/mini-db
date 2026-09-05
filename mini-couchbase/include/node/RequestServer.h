#pragma once

#include "common/Types.h"
#include "node/PartitionManager.h"

class RequestServer {
public:
    RequestServer(
        const PartitionManager& partitionManager,
        int partitionCount
    );

    void handleRequest(const Request& request);

private:
    const PartitionManager& partitionManager;
    int partitionCount;

    int findPartition(const std::string& key) const;
};