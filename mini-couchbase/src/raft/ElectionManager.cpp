#include "raft/ElectionManager.h"

#include <optional>
#include <string>

#include "raft/RaftNetwork.h"

ElectionManager::ElectionManager(
    RaftGroup& group,
    RaftNetwork& network
)
    : group(group),
      network(network),
      votesReceived(0) {
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