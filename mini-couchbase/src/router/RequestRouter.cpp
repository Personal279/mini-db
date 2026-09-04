#include "router/RequestRouter.h"

#include <functional>
#include <stdexcept>

RequestRouter::RequestRouter(
    const ClusterMetadata& metadata,
    int partitionCount
)
    : metadata(metadata),
      partitionCount(partitionCount) {
}

int RequestRouter::findPartition(const std::string& key) const {
    std::size_t hashValue = std::hash<std::string>{}(key);

    return static_cast<int>(hashValue % partitionCount);
}

int RequestRouter::findNode(const std::string& key) const {
    int partitionId = findPartition(key);

    const Partition* partition =
        metadata.getPartition(partitionId);

    if (partition == nullptr) {
        throw std::runtime_error("Partition not found");
    }

    return partition->getActiveNodeId();
}