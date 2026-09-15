#pragma once

#include <string>
#include <vector>

#include "common/Types.h"
#include "storage/StorageEngine.h"

class RaftGroup;

class PartitionManager {
public:
    PartitionManager(
        StorageEngine& storageEngine
    );

    void setRaftGroup(
        RaftGroup& raftGroup
    );

    void addPartition(
        int partitionId
    );

    bool ownsPartition(
        int partitionId
    ) const;

    void processRequest(
        const Request& request
    );

    void applyCommand(
        const std::string& command
    );

private:
    std::vector<int> partitions;

    StorageEngine& storageEngine;

    RaftGroup* raftGroup;
};