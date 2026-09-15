#pragma once

#include <atomic>
#include <thread>

#include "node/PartitionManager.h"
#include "raft/AppendEntries.h"
#include "raft/RaftGroup.h"

class RaftNetwork;

class LeaderManager {
public:
    LeaderManager(
        RaftGroup& group,
        RaftNetwork& network,
        PartitionManager& partitionManager
    );

    ~LeaderManager();

    void sendHeartbeats();

    void startHeartbeatLoop();

    void stopHeartbeatLoop();

private:
    RaftGroup& group;
    RaftNetwork& network;
    PartitionManager& partitionManager;

    std::atomic<bool> heartbeatRunning;
    std::thread heartbeatThread;

    AppendEntriesRequest createRequest(
        int peerId
    ) const;

    void processResponse(
        int peerId,
        const AppendEntriesResponse& response
    );

    void advanceCommitIndex();

    void applyCommittedEntries();

    void heartbeatLoop();
};