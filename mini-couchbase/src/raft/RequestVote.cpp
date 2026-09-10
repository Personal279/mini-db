#include "raft/RequestVote.h"

RequestVoteHandler::RequestVoteHandler(
    RaftGroup& group
)
    : group(group) {
}

RequestVoteResponse RequestVoteHandler::handle(
    const RequestVoteRequest& request
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
        group.getVotedFor() != -1 &&
        group.getVotedFor() != request.candidateId
    ) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    if (!isCandidateLogUpToDate(
            request.lastLogIndex,
            request.lastLogTerm
        )) {
        return {
            group.getCurrentTerm(),
            false
        };
    }

    group.setVotedFor(
        request.candidateId
    );

    return {
        group.getCurrentTerm(),
        true
    };
}

bool RequestVoteHandler::isCandidateLogUpToDate(
    int lastLogIndex,
    int lastLogTerm
) const {
    const RaftLog& log =
        group.getLog();

    int localLastTerm =
        log.getLastTerm();

    int localLastIndex =
        log.getLastIndex();

    if (lastLogTerm != localLastTerm) {
        return lastLogTerm > localLastTerm;
    }

    return lastLogIndex >= localLastIndex;
}