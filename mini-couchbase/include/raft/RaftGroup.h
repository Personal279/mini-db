#pragma once

#include <vector>

#include "raft/RaftLog.h"

enum class RaftRole {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

class RaftGroup {
public:
    RaftGroup(
        int nodeId,
        const std::vector<int>& peerNodeIds
    );

    int getNodeId() const;

    const std::vector<int>& getPeerNodeIds() const;

    int getCurrentTerm() const;

    void setCurrentTerm(int term);

    int getVotedFor() const;

    void setVotedFor(int nodeId);

    RaftRole getRole() const;

    void becomeFollower(int term);

    void becomeCandidate();

    void becomeLeader();

    RaftLog& getLog();

    const RaftLog& getLog() const;

    int getCommitIndex() const;

    void setCommitIndex(int index);

    int getLastApplied() const;

    void setLastApplied(int index);

private:
    int nodeId;

    std::vector<int> peerNodeIds;

    int currentTerm;

    int votedFor;

    RaftRole role;

    RaftLog raftLog;

    int commitIndex;

    int lastApplied;
};