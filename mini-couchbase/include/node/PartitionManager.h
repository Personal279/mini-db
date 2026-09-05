#pragma once

#include <vector>

#include "common/Types.h"
#include "storage/StorageEngine.h"

class PartitionManager {
public:
    PartitionManager(StorageEngine& storageEngine);

    void addPartition(int partitionId);

    bool ownsPartition(int partitionId) const;

    void processRequest(const Request& request);

private:
    std::vector<int> partitions;

    StorageEngine& storageEngine;
};