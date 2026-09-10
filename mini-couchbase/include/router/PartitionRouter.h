#pragma once

#include <string>

class PartitionRouter {
public:
    explicit PartitionRouter(int partitionCount);

    int findPartition(const std::string& key) const;

private:
    int partitionCount;
};