#pragma once

#include <vector>

class Partition {
public:
    Partition(
        int id,
        const std::vector<int>& replicaNodeIds
    );

    int getId() const;

    const std::vector<int>& getReplicaNodeIds() const;

    int getLeaderNodeId() const;
    void setLeaderNodeId(int nodeId);

    bool hasReplica(int nodeId) const;

private:
    int id;

    std::vector<int> replicaNodeIds;

    int leaderNodeId = -1;
};