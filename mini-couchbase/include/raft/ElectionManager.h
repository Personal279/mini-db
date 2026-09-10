#pragma once

#include "raft/RaftGroup.h"
#include "raft/RequestVote.h"

class RaftNetwork;

class ElectionManager {
public:
    ElectionManager(
        RaftGroup& group,
        RaftNetwork& network
    );

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
};