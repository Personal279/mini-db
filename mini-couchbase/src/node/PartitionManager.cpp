#include "node/PartitionManager.h"

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