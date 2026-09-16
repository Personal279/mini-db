#pragma once

#include <string>
#include <vector>

#include "raft/LogEntry.h"
#include "raft/RaftGroup.h"

struct AppendEntriesRequest {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;

    std::vector<LogEntry> entries;

    int leaderCommit;
};

struct AppendEntriesResponse {
    int term;
    bool success;
};

class AppendEntriesHandler {
public:
    explicit AppendEntriesHandler(
        RaftGroup& group
    );

    AppendEntriesResponse handle(
        const AppendEntriesRequest& request
    );

private:
    RaftGroup& group;

    bool matchesPreviousLog(
        int prevLogIndex,
        int prevLogTerm
    ) const;
};