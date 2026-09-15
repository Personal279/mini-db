#include "raft/ElectionManager.h"

#include <chrono>
#include <optional>
#include <string>
#include <thread>

#include "raft/RaftNetwork.h"

ElectionManager::ElectionManager(
    RaftGroup& group,
    RaftNetwork& network
)
    : group(group),
      network(network),
      votesReceived(0),
      heartbeatRunning(false) {
}

ElectionManager::~ElectionManager() {
    stopHeartbeats();
}

RequestVoteRequest
ElectionManager::createRequest() const {
    return {
        group.getCurrentTerm(),
        group.getNodeId(),
        group.getLog().getLastIndex(),
        group.getLog().getLastTerm()
    };
}

void ElectionManager::startElection() {
    stopHeartbeats();

    group.becomeCandidate();

    votesReceived = 1;
}

bool ElectionManager::requestVotes() {
    if (
        group.getRole() !=
        RaftRole::CANDIDATE
    ) {
        return false;
    }

    RequestVoteRequest request =
        createRequest();

    for (
        int peerId :
        group.getPeerNodeIds()
    ) {
        std::string host =
            "127.0.0.1";

        int port =
            9001 + peerId;

        std::optional<RequestVoteResponse>
            response =
                network.sendRequestVote(
                    host,
                    port,
                    request
                );

        if (!response.has_value()) {
            continue;
        }

        if (
            processVote(
                response.value()
            )
        ) {
            return true;
        }
    }

    return false;
}

bool ElectionManager::processVote(
    const RequestVoteResponse& response
) {
    if (
        response.term >
        group.getCurrentTerm()
    ) {
        stopHeartbeats();

        group.becomeFollower(
            response.term
        );

        votesReceived = 0;

        return false;
    }

    if (
        group.getRole() !=
        RaftRole::CANDIDATE
    ) {
        return false;
    }

    if (
        response.term !=
        group.getCurrentTerm()
    ) {
        return false;
    }

    if (!response.voteGranted) {
        return false;
    }

    votesReceived++;

    if (
        votesReceived >=
        getMajority()
    ) {
        group.becomeLeader();

        startHeartbeats();

        return true;
    }

    return false;
}

int ElectionManager::getVotesReceived() const {
    return votesReceived;
}

int ElectionManager::getMajority() const {
    int clusterSize =
        static_cast<int>(
            group.getPeerNodeIds().size()
        ) + 1;

    return (clusterSize / 2) + 1;
}

void ElectionManager::startHeartbeats() {
    if (heartbeatRunning.load()) {
        return;
    }

    heartbeatRunning = true;

    heartbeatThread =
        std::thread(
            &ElectionManager::heartbeatLoop,
            this
        );
}

void ElectionManager::stopHeartbeats() {
    heartbeatRunning = false;

    if (
        heartbeatThread.joinable() &&
        heartbeatThread.get_id() !=
            std::this_thread::get_id()
    ) {
        heartbeatThread.join();
    }
}

void ElectionManager::heartbeatLoop() {
    while (
        heartbeatRunning.load()
    ) {
        if (
            group.getRole() !=
            RaftRole::LEADER
        ) {
            heartbeatRunning = false;
            break;
        }

        AppendEntriesRequest request{
            group.getCurrentTerm(),
            group.getNodeId(),
            group.getLog().getLastIndex(),
            group.getLog().getLastTerm(),
            group.getCommitIndex()
        };

        for (
            int peerId :
            group.getPeerNodeIds()
        ) {
            if (
                !heartbeatRunning.load()
            ) {
                break;
            }

            std::string host =
                "127.0.0.1";

            int port =
                9001 + peerId;

            network.sendAppendEntries(
                host,
                port,
                request
            );
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)
        );
    }
}