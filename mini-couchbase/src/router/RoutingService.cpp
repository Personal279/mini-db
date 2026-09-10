#include "router/RoutingService.h"

#include <stdexcept>

RoutingService::RoutingService(
    PartitionRouter& partitionRouter,
    ClusterMetadata& clusterMetadata
)
    : partitionRouter(partitionRouter),
      clusterMetadata(clusterMetadata) {
}

int RoutingService::findPartition(
    const std::string& key
) const {

    return partitionRouter.findPartition(key);
}

int RoutingService::findLeader(
    int partitionId
) const {

    const Node* leader =
        clusterMetadata.getLeaderNode(partitionId);

    if (leader == nullptr) {
        throw std::runtime_error(
            "Leader node not found for partition"
        );
    }

    return leader->getId();
}

int RoutingService::route(
    const Request& request
) const {

    int partitionId =
        findPartition(request.key);

    return findLeader(partitionId);
}