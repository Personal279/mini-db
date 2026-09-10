#include "router/PartitionRouter.h"

#include <functional>
#include <stdexcept>

PartitionRouter::PartitionRouter(int partitionCount)
    : partitionCount(partitionCount) {

    if (partitionCount <= 0) {
        throw std::invalid_argument(
            "Partition count must be greater than zero"
        );
    }
}

int PartitionRouter::findPartition(
    const std::string& key
) const {
    std::size_t hashValue =
        std::hash<std::string>{}(key);

    return static_cast<int>(
        hashValue % partitionCount
    );
}