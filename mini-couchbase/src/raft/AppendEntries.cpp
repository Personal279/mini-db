#include "raft/AppendEntries.h"

AppendEntriesHandler::AppendEntriesHandler(
    RaftGroup& group
)
    : group(group) {
}

AppendEntriesResponse AppendEntriesHandler::handle(
    const AppendEntriesRequest& request
) {
    if (
        request.term <
        group.getCurrentTerm()
    ) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    if (
        request.term >
        group.getCurrentTerm()
    ) {
        group.becomeFollower(
            request.term
        );
    }

    if (
        group.getRole() ==
        RaftRole::CANDIDATE
    ) {
        group.becomeFollower(
            group.getCurrentTerm()
        );
    }

    if (
        !matchesPreviousLog(
            request.prevLogIndex,
            request.prevLogTerm
        )
    ) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    for (
        const LogEntry& entry :
        request.entries
    ) {
        const LogEntry* existing =
            group.getLog().get(
                entry.index
            );

        if (
            existing != nullptr &&
            existing->term != entry.term
        ) {
            group.getLog().removeFrom(
                entry.index
            );
        }

        if (
            group.getLog().get(
                entry.index
            ) == nullptr
        ) {
            group.getLog().append(
                entry
            );
        }
    }

    if (
        request.leaderCommit >
        group.getCommitIndex()
    ) {
        int newCommitIndex =
            request.leaderCommit;

        if (
            newCommitIndex >
            group.getLog().getLastIndex()
        ) {
            newCommitIndex =
                group.getLog().getLastIndex();
        }

        group.setCommitIndex(
            newCommitIndex
        );
    }

    return {
        group.getCurrentTerm(),
        true
    };
}

bool AppendEntriesHandler::matchesPreviousLog(
    int prevLogIndex,
    int prevLogTerm
) const {
    if (prevLogIndex == 0) {
        return true;
    }

    const LogEntry* entry =
        group.getLog().get(
            prevLogIndex
        );

    if (entry == nullptr) {
        return false;
    }

    return entry->term == prevLogTerm;
}