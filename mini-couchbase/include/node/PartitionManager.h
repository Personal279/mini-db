#pragma once

#include <vector>

#include "common/Types.h"
#include "index/IndexClient.h"
#include "raft/RaftGroup.h"
#include "storage/StorageEngine.h"

class PartitionManager {
public:
    PartitionManager(
        StorageEngine& storageEngine,
        RaftGroup& raftGroup
    );

    void addPartition(int partitionId);
    bool ownsPartition(int partitionId) const;

    Response processRequest(const Request& request);
    void applyCommand(const std::string& command);

private:
    StorageEngine& storageEngine;
    RaftGroup& raftGroup;
    IndexClient indexClient;
    std::vector<int> partitions;

    void appendWriteCommand(const Request& request);
};
