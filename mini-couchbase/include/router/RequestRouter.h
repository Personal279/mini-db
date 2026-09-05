#pragma once

#include "cluster/ClusterMetadata.h"
#include "common/Types.h"

class RequestRouter {
public:
    RequestRouter(const ClusterMetadata& metadata, int partitionCount);

    int findPartition(const std::string& key) const;
    int findNode(const std::string& key) const;

    int route(const Request& request) const;

private:
    const ClusterMetadata& metadata;
    int partitionCount;
};