#include "raft/RaftGroup.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    // One Raft group: node 1, partition 0, peers 2 and 3.
    RaftGroup group(1, 0, std::vector<int>{2, 3});

    // A new group must begin without a committed log entry.
    assert(group.getCommitIndex() == 0);

    // The group must contain exactly the configured peers.
    const auto& peers = group.getPeerNodeIds();

    assert(peers.size() == 2);
    assert(peers[0] == 2);
    assert(peers[1] == 3);

    // Acknowledgement progress for different peers is tracked
    // independently.
    group.setMatchIndex(2, 0);
    group.setMatchIndex(3, 0);

    assert(group.getMatchIndex(2) == 0);
    assert(group.getMatchIndex(3) == 0);

    // Updating one peer must not affect the other peer.
    group.setMatchIndex(2, 5);

    assert(group.getMatchIndex(2) == 5);
    assert(group.getMatchIndex(3) == 0);

    // Next-index progress must also be tracked independently.
    group.setNextIndex(2, 6);
    group.setNextIndex(3, 1);

    assert(group.getNextIndex(2) == 6);
    assert(group.getNextIndex(3) == 1);

    std::cout << "Raft group peer/progress tests passed.\n";

    return 0;
}