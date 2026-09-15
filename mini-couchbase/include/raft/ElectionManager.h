#pragma once

#include <atomic>
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

private:
    RaftGroup& group;
    RaftNetwork& network;

    int votesReceived;

    std::atomic<bool> heartbeatRunning;
    std::thread heartbeatThread;

    void startHeartbeats();

    void stopHeartbeats();

    void heartbeatLoop();
};