#include "raft/AppendEntries.h"

#include <iostream>

AppendEntriesHandler::AppendEntriesHandler(
    RaftGroup& group
)
    : group(group) {
}

AppendEntriesResponse
AppendEntriesHandler::handle(
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
        group.getRole() !=
        RaftRole::FOLLOWER
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

    RaftLog& log =
        group.getLog();

    for (
        const LogEntry& entry :
        request.entries
    ) {

        const LogEntry* existing =
            log.get(entry.index);

        if (
            existing != nullptr
        ) {

            if (
                existing->term !=
                entry.term
            ) {

                log.removeFrom(
                    entry.index
                );

                log.append(
                    entry
                );
            }

        } else {

            log.append(
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
            log.getLastIndex()
        ) {

            newCommitIndex =
                log.getLastIndex();
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

bool
AppendEntriesHandler::matchesPreviousLog(
    int prevLogIndex,
    int prevLogTerm
) const {

    if (
        prevLogIndex == 0
    ) {
        return true;
    }

    const RaftLog& log =
        group.getLog();

    const LogEntry* entry =
        log.get(prevLogIndex);

    if (
        entry == nullptr
    ) {
        return false;
    }

    return (
        entry->term ==
        prevLogTerm
    );
}