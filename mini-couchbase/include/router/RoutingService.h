#pragma once

#include <string>

#include "cluster/ClusterMetadata.h"
#include "common/Types.h"
#include "router/PartitionRouter.h"

class RoutingService {
public:
    RoutingService(
        PartitionRouter& partitionRouter,
        ClusterMetadata& clusterMetadata
    );

    int findPartition(
        const std::string& key
    ) const;

    int findLeader(
        int partitionId
    ) const;

    int route(
        const Request& request
    ) const;

private:
    PartitionRouter& partitionRouter;
    ClusterMetadata& clusterMetadata;
};