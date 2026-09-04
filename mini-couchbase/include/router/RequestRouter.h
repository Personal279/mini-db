#pragma once

#include <string>

#include "cluster/ClusterMetadata.h"

class RequestRouter {
public:
    RequestRouter(const ClusterMetadata& metadata, int partitionCount);

    int findPartition(const std::string& key) const;
    int findNode(const std::string& key) const;

private:
    const ClusterMetadata& metadata;
    int partitionCount;
};