#include "cluster/ClusterMetadata.h"

#include <stdexcept>

void ClusterMetadata::addNode(
    const Node& node
) {
    nodes.push_back(node);
}

void ClusterMetadata::addPartition(
    const Partition& partition
) {
    partitions.push_back(partition);
}

const Node* ClusterMetadata::getNode(
    int nodeId
) const {

    for (const auto& node : nodes) {

        if (node.getId() == nodeId) {
            return &node;
        }
    }

    return nullptr;
}

const Partition* ClusterMetadata::getPartition(
    int partitionId
) const {

    for (const auto& partition : partitions) {

        if (partition.getId() == partitionId) {
            return &partition;
        }
    }

    return nullptr;
}

const Node* ClusterMetadata::getLeaderNode(
    int partitionId
) const {

    const Partition* partition =
        getPartition(partitionId);

    if (partition == nullptr) {
        return nullptr;
    }

    return getNode(
        partition->getLeaderNodeId()
    );
}

void ClusterMetadata::updateLeader(
    int partitionId,
    int nodeId
) {

    for (auto& partition : partitions) {

        if (partition.getId() == partitionId) {

            partition.setLeaderNodeId(nodeId);

            return;
        }
    }

    throw std::runtime_error(
        "Partition not found"
    );
}