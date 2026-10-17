
#pragma once

#include <atomic>
#include <functional>
#include <thread>

#include "raft/RaftGroup.h"
#include "raft/RequestVote.h"

class RaftNetwork;

class ElectionManager {
public:
    ElectionManager(
        RaftGroup& group,
        RaftNetwork& network
    );

    ~ElectionManager();

    RequestVoteRequest createRequest() const;

    void startElection();

    bool requestVotes();

    bool processVote(
        const RequestVoteResponse& response
    );

    int getVotesReceived() const;

    int getMajority() const;

    void resetElectionTimer();

    void startElectionTimer();

    void stopElectionTimer();

    // Called when this node successfully becomes leader.
    void setLeaderChangeCallback(
        std::function<void(int partitionId, int nodeId)> callback
    );

private:
    RaftGroup& group;
    RaftNetwork& network;
    std::function<void(int, int)> leaderChangeCallback;

    std::atomic<int> votesReceived;
    std::atomic<bool> electionTimerRunning;
    std::atomic<long long> lastHeartbeatTime;

    std::thread electionTimerThread;

    void sendHeartbeats();

    void electionTimerLoop();

    long long getCurrentTimeMs() const;

    int getElectionTimeoutMs() const;
};
