#include "raft/LeaderManager.h"

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "raft/RaftNetwork.h"

LeaderManager::LeaderManager(
    RaftGroup& group,
    RaftNetwork& network,
    PartitionManager& partitionManager
)
    : group(group),
      network(network),
      partitionManager(partitionManager),
      heartbeatRunning(false) {
}

LeaderManager::~LeaderManager() {
    stopHeartbeatLoop();
}

void LeaderManager::sendHeartbeats() {
    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    for (
        int peerId :
        group.getPeerNodeIds()
    ) {
        int port =
            9001 + peerId;

        AppendEntriesRequest request =
            createRequest(peerId);

        std::optional<AppendEntriesResponse>
            response =
                network.sendAppendEntries(
                    "127.0.0.1",
                    port,
                    request
                );

        if (!response.has_value()) {
            continue;
        }

        processResponse(
            peerId,
            response.value()
        );

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

void LeaderManager::startHeartbeatLoop() {
    if (
        heartbeatRunning.load()
    ) {
        return;
    }

    heartbeatRunning = true;

    heartbeatThread =
        std::thread(
            &LeaderManager::heartbeatLoop,
            this
        );
}

void LeaderManager::stopHeartbeatLoop() {
    heartbeatRunning = false;

    if (
        heartbeatThread.joinable() &&
        heartbeatThread.get_id() !=
            std::this_thread::get_id()
    ) {
        heartbeatThread.join();
    }
}

void LeaderManager::heartbeatLoop() {
    while (
        heartbeatRunning.load()
    ) {
        if (
            group.getRole() ==
            RaftRole::LEADER
        ) {
            sendHeartbeats();
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }
}

AppendEntriesRequest
LeaderManager::createRequest(
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

    const std::vector<LogEntry>&
        logEntries =
            group.getLog().entries();

    for (
        const LogEntry& entry :
        logEntries
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

void LeaderManager::processResponse(
    int peerId,
    const AppendEntriesResponse& response
) {
    if (
        response.term >
        group.getCurrentTerm()
    ) {
        group.becomeFollower(
            response.term
        );

        return;
    }

    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    if (!response.success) {
        int currentNextIndex =
            group.getNextIndex(peerId);

        if (
            currentNextIndex > 1
        ) {
            group.setNextIndex(
                peerId,
                currentNextIndex - 1
            );
        }

        return;
    }

    int lastLogIndex =
        group.getLog().getLastIndex();

    /*
     * If the follower accepted the
     * AppendEntries, it has replicated
     * everything we sent.
     */
    group.setMatchIndex(
        peerId,
        lastLogIndex
    );

    group.setNextIndex(
        peerId,
        lastLogIndex + 1
    );
}

void LeaderManager::advanceCommitIndex() {
    if (
        group.getRole() !=
        RaftRole::LEADER
    ) {
        return;
    }

    int lastLogIndex =
        group.getLog().getLastIndex();

    int currentCommitIndex =
        group.getCommitIndex();

    int clusterSize =
        static_cast<int>(
            group.getPeerNodeIds().size()
        ) + 1;

    int majority =
        (clusterSize / 2) + 1;

    for (
        int index =
            currentCommitIndex + 1;
        index <= lastLogIndex;
        index++
    ) {
        const LogEntry* entry =
            group.getLog().get(index);

        if (entry == nullptr) {
            continue;
        }

        /*
         * Only an entry from the current
         * term can be committed using
         * this majority rule.
         */
        if (
            entry->term !=
            group.getCurrentTerm()
        ) {
            continue;
        }

        /*
         * Leader has its own copy.
         */
        int replicatedCount = 1;

        for (
            int peerId :
            group.getPeerNodeIds()
        ) {
            if (
                group.getMatchIndex(peerId)
                >= index
            ) {
                replicatedCount++;
            }
        }

        if (
            replicatedCount >=
            majority
        ) {
            group.setCommitIndex(
                index
            );
        }
    }
}

void LeaderManager::applyCommittedEntries() {
    while (
        group.getLastApplied() <
        group.getCommitIndex()
    ) {
        int nextIndex =
            group.getLastApplied() + 1;

        const LogEntry* entry =
            group.getLog().get(
                nextIndex
            );

        if (entry == nullptr) {
            break;
        }

        partitionManager.applyCommand(
            entry->command
        );

        group.setLastApplied(
            nextIndex
        );
    }
}