#pragma once

#include <atomic>
#include <unordered_map>
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
        int partitionId,
        const std::vector<int>& peerNodeIds
    );

    int getNodeId() const;
    int getPartitionId() const;
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

    int getNextIndex(int nodeId) const;

    void setNextIndex(
        int nodeId,
        int index
    );

    int getMatchIndex(int nodeId) const;

    void setMatchIndex(
        int nodeId,
        int index
    );

private:
    int nodeId;
    int partitionId;  // Add this line

    std::vector<int> peerNodeIds;

    // Read by the HTTP thread, written by replication/election/RPC threads.
    std::atomic<int> currentTerm;

    int votedFor;

    std::atomic<RaftRole> role;

    RaftLog raftLog;

    std::atomic<int> commitIndex;

    // Written by the replication thread after a command is applied;
    // polled by the HTTP thread to know when a write is visible.
    std::atomic<int> lastApplied;

    std::unordered_map<int, int> nextIndex;

    std::unordered_map<int, int> matchIndex;
};