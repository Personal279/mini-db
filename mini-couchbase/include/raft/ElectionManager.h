#pragma once

#include <atomic>
#include <chrono>
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

private:
    RaftGroup& group;
    RaftNetwork& network;

    int votesReceived;

    std::atomic<bool> electionTimerRunning;
    std::atomic<long long> lastHeartbeatTime;

    std::thread electionTimerThread;

    void electionTimerLoop();

    long long getCurrentTimeMs() const;

    int getElectionTimeoutMs() const;
};