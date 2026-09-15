#pragma once

#include <atomic>
#include <thread>

#include "node/PartitionManager.h"
#include "raft/AppendEntries.h"
#include "raft/RaftGroup.h"

class RaftNetwork;

class RaftReplicationManager {
public:
    RaftReplicationManager(
        RaftGroup& group,
        RaftNetwork& network,
        PartitionManager& partitionManager
    );

    ~RaftReplicationManager();

    void start();

    void stop();

    void replicateOnce();

private:
    RaftGroup& group;
    RaftNetwork& network;
    PartitionManager& partitionManager;

    std::atomic<bool> running;
    std::thread replicationThread;

    int lastApplied;

    void replicationLoop();

    void replicateToPeer(
        int peerId
    );

    void advanceCommitIndex();

    void applyCommittedEntries();

    AppendEntriesRequest buildRequest(
        int peerId
    ) const;
};