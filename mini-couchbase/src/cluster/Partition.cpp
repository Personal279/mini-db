#include "cluster/Partition.h"

Partition::Partition(int id, int activeNodeId)
    : id(id), activeNodeId(activeNodeId) {
}

int Partition::getId() const {
    return id;
}

int Partition::getActiveNodeId() const {
    return activeNodeId;
}