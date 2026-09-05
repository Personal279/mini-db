#pragma once

#include <vector>

class PartitionManager {
public:
    void addPartition(int partitionId);

    bool ownsPartition(int partitionId) const;

private:
    std::vector<int> partitions;
};