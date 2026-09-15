#include "raft/AppendEntries.h"

AppendEntriesHandler::AppendEntriesHandler(
    RaftGroup& group
)
    : group(group) {
}

AppendEntriesResponse AppendEntriesHandler::handle(
    const AppendEntriesRequest& request
) {
    if (request.term < group.getCurrentTerm()) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    if (request.term > group.getCurrentTerm()) {
        group.becomeFollower(request.term);
    }

    if (
        request.prevLogIndex >= 0 &&
        request.prevLogIndex > group.getLog().getLastIndex()
    ) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    if (request.leaderCommit > group.getCommitIndex()) {
        int commitIndex =
            request.leaderCommit;

        int lastLogIndex =
            group.getLog().getLastIndex();

        if (commitIndex > lastLogIndex) {
            commitIndex = lastLogIndex;
        }

        group.setCommitIndex(commitIndex);
    }

    return {
        group.getCurrentTerm(),
        true
    };
}