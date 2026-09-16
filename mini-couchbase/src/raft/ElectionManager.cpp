#include "raft/ElectionManager.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <random>
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
      electionTimerRunning(false),
      lastHeartbeatTime(getCurrentTimeMs()) {

    network.setHeartbeatCallback(
        [this]() {
            resetElectionTimer();
        }
    );
}

ElectionManager::~ElectionManager() {
    stopElectionTimer();
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

    if (
        group.getRole() ==
        RaftRole::LEADER
    ) {
        return;
    }

    group.becomeCandidate();

    votesReceived = 1;

    resetElectionTimer();

    std::cout
        << "Node "
        << group.getNodeId()
        << " started election for term "
        << group.getCurrentTerm()
        << std::endl;
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

        resetElectionTimer();

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

    std::cout
        << "Node "
        << group.getNodeId()
        << " received vote. Votes: "
        << votesReceived
        << "/"
        << getMajority()
        << std::endl;

    if (
        votesReceived >=
        getMajority()
    ) {

        group.becomeLeader();

        resetElectionTimer();

        std::cout
            << "Node "
            << group.getNodeId()
            << " became LEADER for term "
            << group.getCurrentTerm()
            << std::endl;

        sendHeartbeats();

        return true;
    }

    return false;
}

int ElectionManager::getVotesReceived() const {
    return votesReceived.load();
}

int ElectionManager::getMajority() const {

    int clusterSize =
        static_cast<int>(
            group.getPeerNodeIds().size()
        ) + 1;

    return (
        clusterSize / 2
    ) + 1;
}

void ElectionManager::resetElectionTimer() {

    lastHeartbeatTime =
        getCurrentTimeMs();
}

void ElectionManager::startElectionTimer() {

    if (
        electionTimerRunning.load()
    ) {
        return;
    }

    electionTimerRunning = true;

    resetElectionTimer();

    electionTimerThread =
        std::thread(
            &ElectionManager::electionTimerLoop,
            this
        );
}

void ElectionManager::stopElectionTimer() {

    electionTimerRunning = false;

    if (
        electionTimerThread.joinable() &&
        electionTimerThread.get_id() !=
            std::this_thread::get_id()
    ) {

        electionTimerThread.join();
    }
}

void ElectionManager::electionTimerLoop() {

    int electionTimeout =
        getElectionTimeoutMs();

    while (
        electionTimerRunning.load()
    ) {

        if (
            group.getRole() ==
            RaftRole::LEADER
        ) {

            sendHeartbeats();

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );

            continue;
        }

        long long now =
            getCurrentTimeMs();

        long long elapsed =
            now -
            lastHeartbeatTime.load();

        if (
            elapsed >=
            electionTimeout
        ) {

            startElection();

            requestVotes();

            electionTimeout =
                getElectionTimeoutMs();

            resetElectionTimer();
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }
}

void ElectionManager::sendHeartbeats() {

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

        std::string host =
            "127.0.0.1";

        int port =
            9001 + peerId;

        int prevLogIndex =
            group.getNextIndex(peerId) - 1;

        int prevLogTerm = 0;

        if (
            prevLogIndex > 0
        ) {

            const LogEntry* previousEntry =
                group.getLog().get(
                    prevLogIndex
                );

            if (
                previousEntry != nullptr
            ) {

                prevLogTerm =
                    previousEntry->term;
            }
        }

        AppendEntriesRequest request{
            group.getCurrentTerm(),
            group.getNodeId(),
            prevLogIndex,
            prevLogTerm,
            {},
            group.getCommitIndex()
        };

        std::optional<AppendEntriesResponse>
            response =
                network.sendAppendEntries(
                    host,
                    port,
                    request
                );

        if (
            response.has_value() &&
            response->term >
                group.getCurrentTerm()
        ) {

            group.becomeFollower(
                response->term
            );

            resetElectionTimer();

            return;
        }
    }
}

long long
ElectionManager::getCurrentTimeMs() const {

    return std::chrono::duration_cast<
        std::chrono::milliseconds
    >(
        std::chrono::steady_clock::now()
            .time_since_epoch()
    ).count();
}

int
ElectionManager::getElectionTimeoutMs() const {

    static thread_local std::mt19937 generator(
        std::random_device{}()
    );

    std::uniform_int_distribution<int>
        distribution(
            1500,
            3000
        );

    return distribution(
        generator
    );
}