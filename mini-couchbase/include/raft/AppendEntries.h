#pragma once

#include "raft/RaftGroup.h"

struct AppendEntriesRequest {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;
    int leaderCommit;
};

struct AppendEntriesResponse {
    int term;
    bool success;
};

class AppendEntriesHandler {
public:
    explicit AppendEntriesHandler(RaftGroup& group);

    AppendEntriesResponse handle(
        const AppendEntriesRequest& request
    );

private:
    RaftGroup& group;
};