#include "cluster/Partition.h"

#include <algorithm>
#include <stdexcept>

Partition::Partition(
    int id,
    const std::vector<int>& replicaNodeIds
)
    : id(id),
      replicaNodeIds(replicaNodeIds) {

    if (replicaNodeIds.empty()) {
        throw std::invalid_argument(
            "Partition must have at least one replica"
        );
    }

    leaderNodeId = replicaNodeIds.front();
}

int Partition::getId() const {
    return id;
}

const std::vector<int>& Partition::getReplicaNodeIds() const {
    return replicaNodeIds;
}

int Partition::getLeaderNodeId() const {
    return leaderNodeId;
}

void Partition::setLeaderNodeId(int nodeId) {
    if (!hasReplica(nodeId)) {
        throw std::invalid_argument(
            "Leader must be one of the partition replicas"
        );
    }

    leaderNodeId = nodeId;
}

bool Partition::hasReplica(int nodeId) const {
    return std::find(
        replicaNodeIds.begin(),
        replicaNodeIds.end(),
        nodeId
    ) != replicaNodeIds.end();
}