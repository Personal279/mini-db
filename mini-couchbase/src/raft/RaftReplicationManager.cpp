#include "raft/RaftReplicationManager.h"

#include <chrono>
#include <optional>
#include <thread>
#include <vector>
#include <iostream>

#include "raft/RaftNetwork.h"

RaftReplicationManager::RaftReplicationManager(
    RaftGroup& group,
    RaftNetwork& network,
    PartitionManager& partitionManager
)
    : group(group),
      network(network),
      partitionManager(partitionManager),
      running(false),
      lastApplied(0) {
}

RaftReplicationManager::~RaftReplicationManager() {
    stop();
}

void RaftReplicationManager::start() {
    if (running.load()) {
        return;
    }

    running = true;

    replicationThread =
        std::thread(
            &RaftReplicationManager::replicationLoop,
            this
        );
}

void RaftReplicationManager::stop() {
    running = false;

    if (
        replicationThread.joinable() &&
        replicationThread.get_id() !=
            std::this_thread::get_id()
    ) {
        replicationThread.join();
    }
}

void RaftReplicationManager::replicateOnce() {
    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        applyCommittedEntries();
        return;
    }

    for (
        int peerId :
        group.getPeerNodeIds()
    ) {
        replicateToPeer(peerId);

        if (
            group.getRole() !=
            RaftRole::LEADER
        ) {
            return;
        }
    }

    advanceCommitIndex();
    applyCommittedEntries();
}

void RaftReplicationManager::replicationLoop() {
    while (running.load()) {
        replicateOnce();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }
}

void RaftReplicationManager::replicateToPeer(
    int peerId
) {
    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    AppendEntriesRequest request =
        buildRequest(peerId);

        std::cout
            << "Node "
            << group.getNodeId()
            << " sending AppendEntries to Node "
            << peerId
            << ": entries="
            << request.entries.size()
            << " prevLogIndex="
            << request.prevLogIndex
            << " leaderCommit="
            << request.leaderCommit
            << std::endl;

    std::optional<AppendEntriesResponse>
        response =
            network.sendAppendEntries(
                "127.0.0.1",
                9001 + peerId,
                request
            );

    if (!response.has_value()) {
        return;
    }

    if (
        response->term >
        group.getCurrentTerm()
    ) {
        group.becomeFollower(
            response->term
        );

        return;
    }

    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    if (response->success) {
        int replicatedIndex =
            request.prevLogIndex +
            static_cast<int>(
                request.entries.size()
            );

        group.setMatchIndex(
            peerId,
            replicatedIndex
        );

        group.setNextIndex(
            peerId,
            replicatedIndex + 1
        );

        return;
    }

    int nextIndex =
        group.getNextIndex(peerId);

    if (nextIndex > 1) {
        group.setNextIndex(
            peerId,
            nextIndex - 1
        );
    }
}

void RaftReplicationManager::advanceCommitIndex() {
    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    int currentCommit =
        group.getCommitIndex();

    int lastIndex =
        group.getLog().getLastIndex();

    for (
        int index = currentCommit + 1;
        index <= lastIndex;
        ++index
    ) {
        const LogEntry* entry =
            group.getLog().get(index);

        if (entry == nullptr) {
            continue;
        }

        if (
            entry->term !=
            group.getCurrentTerm()
        ) {
            continue;
        }

        int replicatedCount = 1;

        for (
            int peerId :
            group.getPeerNodeIds()
        ) {
            if (
                group.getMatchIndex(peerId) >=
                index
            ) {
                replicatedCount++;
            }
        }

        int clusterSize =
            static_cast<int>(
                group.getPeerNodeIds().size()
            ) + 1;

        int majority =
            (clusterSize / 2) + 1;

        if (
            replicatedCount >=
            majority
        ) {
            group.setCommitIndex(index);
        }
    }
}

void RaftReplicationManager::applyCommittedEntries() {
    int commitIndex =
        group.getCommitIndex();

    int lastLogIndex =
        group.getLog().getLastIndex();

    if (commitIndex > lastLogIndex) {
        commitIndex = lastLogIndex;
    }

    while (
        lastApplied <
        commitIndex
    ) {
        int index =
            lastApplied + 1;

        const LogEntry* entry =
            group.getLog().get(index);

        if (entry == nullptr) {
            break;
        }

        partitionManager.applyCommand(
            group.getPartitionId(),
            entry->command
        );

        lastApplied = index;

        group.setLastApplied(
            lastApplied
        );
    }
}

AppendEntriesRequest
RaftReplicationManager::buildRequest(
    int peerId
) const {
    int nextIndex =
        group.getNextIndex(peerId);

    int prevLogIndex =
        nextIndex - 1;

    int prevLogTerm = 0;

    if (prevLogIndex > 0) {
        const LogEntry* previousEntry =
            group.getLog().get(
                prevLogIndex
            );

        if (previousEntry != nullptr) {
            prevLogTerm =
                previousEntry->term;
        }
    }

    std::vector<LogEntry> entries;

    const std::vector<LogEntry>& log =
        group.getLog().entries();

    for (
        const LogEntry& entry :
        log
    ) {
        if (
            entry.index >=
            nextIndex
        ) {
            entries.push_back(entry);
        }
    }

    return {
        group.getCurrentTerm(),
        group.getNodeId(),
        prevLogIndex,
        prevLogTerm,
        entries,
        group.getCommitIndex()
    };
}