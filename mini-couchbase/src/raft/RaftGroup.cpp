#include "raft/RaftGroup.h"

#include <stdexcept>

RaftGroup::RaftGroup(
    int nodeId,
    const std::vector<int>& peerNodeIds
)
    : nodeId(nodeId),
      peerNodeIds(peerNodeIds),
      currentTerm(0),
      votedFor(-1),
      role(RaftRole::FOLLOWER),
      commitIndex(0),
      lastApplied(0) {

    for (int peerId : peerNodeIds) {
        if (peerId == nodeId) {
            throw std::invalid_argument(
                "Raft group cannot contain itself as a peer"
            );
        }

        nextIndex[peerId] = 1;
        matchIndex[peerId] = 0;
    }
}

int RaftGroup::getNodeId() const {
    return nodeId;
}

const std::vector<int>& RaftGroup::getPeerNodeIds() const {
    return peerNodeIds;
}

int RaftGroup::getCurrentTerm() const {
    return currentTerm;
}

void RaftGroup::setCurrentTerm(int term) {
    if (term < currentTerm) {
        throw std::invalid_argument(
            "Term cannot move backwards"
        );
    }

    currentTerm = term;
}

int RaftGroup::getVotedFor() const {
    return votedFor;
}

void RaftGroup::setVotedFor(int nodeId) {
    votedFor = nodeId;
}

RaftRole RaftGroup::getRole() const {
    return role;
}

void RaftGroup::becomeFollower(int term) {
    if (term < currentTerm) {
        throw std::invalid_argument(
            "Term cannot move backwards"
        );
    }

    currentTerm = term;
    role = RaftRole::FOLLOWER;
    votedFor = -1;
}

void RaftGroup::becomeCandidate() {
    role = RaftRole::CANDIDATE;
    currentTerm++;
    votedFor = nodeId;
}

void RaftGroup::becomeLeader() {
    role = RaftRole::LEADER;

    int next =
        raftLog.getLastIndex() + 1;

    for (int peerId : peerNodeIds) {
        nextIndex[peerId] = next;
        matchIndex[peerId] = 0;
    }
}

RaftLog& RaftGroup::getLog() {
    return raftLog;
}

const RaftLog& RaftGroup::getLog() const {
    return raftLog;
}

int RaftGroup::getCommitIndex() const {
    return commitIndex;
}

void RaftGroup::setCommitIndex(int index) {
    if (index < commitIndex) {
        throw std::invalid_argument(
            "Commit index cannot move backwards"
        );
    }

    commitIndex = index;
}

int RaftGroup::getLastApplied() const {
    return lastApplied;
}

void RaftGroup::setLastApplied(int index) {
    if (index < lastApplied) {
        throw std::invalid_argument(
            "Last applied index cannot move backwards"
        );
    }

    lastApplied = index;
}

int RaftGroup::getNextIndex(int nodeId) const {
    auto it =
        nextIndex.find(nodeId);

    if (it == nextIndex.end()) {
        throw std::invalid_argument(
            "Unknown peer node"
        );
    }

    return it->second;
}

void RaftGroup::setNextIndex(
    int nodeId,
    int index
) {
    if (index < 1) {
        throw std::invalid_argument(
            "Next index must be at least 1"
        );
    }

    auto it =
        nextIndex.find(nodeId);

    if (it == nextIndex.end()) {
        throw std::invalid_argument(
            "Unknown peer node"
        );
    }

    it->second = index;
}

int RaftGroup::getMatchIndex(int nodeId) const {
    auto it =
        matchIndex.find(nodeId);

    if (it == matchIndex.end()) {
        throw std::invalid_argument(
            "Unknown peer node"
        );
    }

    return it->second;
}

void RaftGroup::setMatchIndex(
    int nodeId,
    int index
) {
    if (index < 0) {
        throw std::invalid_argument(
            "Match index cannot be negative"
        );
    }

    auto it =
        matchIndex.find(nodeId);

    if (it == matchIndex.end()) {
        throw std::invalid_argument(
            "Unknown peer node"
        );
    }

    it->second = index;
}