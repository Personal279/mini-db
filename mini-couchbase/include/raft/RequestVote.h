#pragma once

#include "raft/RaftGroup.h"

struct RequestVoteRequest {
    int term;
    int candidateId;
    int lastLogIndex;
    int lastLogTerm;
};

struct RequestVoteResponse {
    int term;
    bool voteGranted;
};

class RequestVoteHandler {
public:
    explicit RequestVoteHandler(RaftGroup& group);

    RequestVoteResponse handle(
        const RequestVoteRequest& request
    );

private:
    RaftGroup& group;

    bool isCandidateLogUpToDate(
        int lastLogIndex,
        int lastLogTerm
    ) const;
};