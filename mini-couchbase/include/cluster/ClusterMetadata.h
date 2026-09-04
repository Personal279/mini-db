#pragma once

#include <vector>
#include "cluster/Node.h"
#include "cluster/Partition.h"

class ClusterMetadata {
public:
    void addNode(const Node& node);
    void addPartition(const Partition& partition);

    const Node* getNode(int nodeId) const;
    const Partition* getPartition(int partitionId) const;

private:
    std::vector<Node> nodes;
    std::vector<Partition> partitions;
};