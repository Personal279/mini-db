
#pragma once

#include <unordered_map>

#include "query/QueryExecutor.h"
#include "common/Types.h"
#include "index/IndexClient.h"
#include "raft/RaftGroup.h"
#include "storage/StorageEngine.h"
#include "router/PartitionRouter.h"

class PartitionManager {
public:
    PartitionManager(
        StorageEngine& storageEngine,
        const PartitionRouter& partitionRouter
    );

    void addPartition(int partitionId, RaftGroup& raftGroup);

    bool ownsPartition(int partitionId) const;

    RaftGroup& getRaftGroup(int partitionId);

    Response processRequest(const Request& request);

    QueryResult executeQuery(const QueryPredicate& predicate);

    void applyCommand(
        int partitionId,
        const std::string& command
    );

private:
    StorageEngine& storageEngine;
    const PartitionRouter& partitionRouter;
    IndexClient indexClient;

    std::unordered_map<int, RaftGroup*> raftGroups;

    int findPartition(const std::string& key) const;

    int appendWriteCommand(
        RaftGroup& raftGroup,
        const Request& request
    );
};
