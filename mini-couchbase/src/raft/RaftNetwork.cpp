#include "raft/RaftNetwork.h"

#include <sstream>

#include "network/RpcCodec.h"

RaftNetwork::RaftNetwork(
    NodeNetwork& network,
    int nodeId,
    RaftGroup& group
)
    : network(network),
      nodeId(nodeId),
      group(group) {

    network.setMessageHandler(
        [this](const RpcMessage& message) {
            return handleMessage(message);
        }
    );
}

std::optional<RequestVoteResponse>
RaftNetwork::sendRequestVote(
    const std::string& host,
    int port,
    const RequestVoteRequest& request
) {
    std::string payload =
        std::to_string(request.term) + "," +
        std::to_string(request.candidateId) + "," +
        std::to_string(request.lastLogIndex) + "," +
        std::to_string(request.lastLogTerm);

    RpcMessage message{
        RpcType::REQUEST_VOTE,
        nodeId,
        -1,
        payload
    };

    std::optional<RpcMessage> response =
        network.send(
            host,
            port,
            message
        );

    if (!response.has_value()) {
        return std::nullopt;
    }

    if (
        response->type !=
        RpcType::VOTE_RESPONSE
    ) {
        return std::nullopt;
    }

    return decodeVoteResponse(
        response.value()
    );
}

std::optional<AppendEntriesResponse>
RaftNetwork::sendAppendEntries(
    const std::string& host,
    int port,
    const AppendEntriesRequest& request
) {
    std::string payload =
        std::to_string(request.term) + "," +
        std::to_string(request.leaderId) + "," +
        std::to_string(request.prevLogIndex) + "," +
        std::to_string(request.prevLogTerm) + "," +
        std::to_string(request.leaderCommit);

    RpcMessage message{
        RpcType::APPEND_ENTRIES,
        nodeId,
        -1,
        payload
    };

    std::optional<RpcMessage> response =
        network.send(
            host,
            port,
            message
        );

    if (!response.has_value()) {
        return std::nullopt;
    }

    if (
        response->type !=
        RpcType::APPEND_ENTRIES_RESPONSE
    ) {
        return std::nullopt;
    }

    return decodeAppendEntriesResponse(
        response.value()
    );
}

std::optional<RpcMessage>
RaftNetwork::handleMessage(
    const RpcMessage& message
) {
    if (
        message.type ==
        RpcType::REQUEST_VOTE
    ) {
        RequestVoteRequest request =
            decodeRequestVote(message);

        RequestVoteHandler handler(
            group
        );

        RequestVoteResponse response =
            handler.handle(request);

        std::string payload =
            std::to_string(response.term) + "," +
            (response.voteGranted ? "1" : "0");

        return RpcMessage{
            RpcType::VOTE_RESPONSE,
            nodeId,
            message.senderId,
            payload
        };
    }

    if (
        message.type ==
        RpcType::APPEND_ENTRIES
    ) {
        AppendEntriesRequest request =
            decodeAppendEntries(message);

        AppendEntriesHandler handler(
            group
        );

        AppendEntriesResponse response =
            handler.handle(request);

        std::string payload =
            std::to_string(response.term) + "," +
            (response.success ? "1" : "0");

        return RpcMessage{
            RpcType::APPEND_ENTRIES_RESPONSE,
            nodeId,
            message.senderId,
            payload
        };
    }

    return std::nullopt;
}

RequestVoteRequest
RaftNetwork::decodeRequestVote(
    const RpcMessage& message
) const {
    std::stringstream stream(
        message.payload
    );

    std::string term;
    std::string candidateId;
    std::string lastLogIndex;
    std::string lastLogTerm;

    std::getline(
        stream,
        term,
        ','
    );

    std::getline(
        stream,
        candidateId,
        ','
    );

    std::getline(
        stream,
        lastLogIndex,
        ','
    );

    std::getline(
        stream,
        lastLogTerm,
        ','
    );

    return {
        std::stoi(term),
        std::stoi(candidateId),
        std::stoi(lastLogIndex),
        std::stoi(lastLogTerm)
    };
}

RequestVoteResponse
RaftNetwork::decodeVoteResponse(
    const RpcMessage& message
) const {
    std::stringstream stream(
        message.payload
    );

    std::string term;
    std::string voteGranted;

    std::getline(
        stream,
        term,
        ','
    );

    std::getline(
        stream,
        voteGranted,
        ','
    );

    return {
        std::stoi(term),
        voteGranted == "1"
    };
}

AppendEntriesRequest
RaftNetwork::decodeAppendEntries(
    const RpcMessage& message
) const {
    std::stringstream stream(
        message.payload
    );

    std::string term;
    std::string leaderId;
    std::string prevLogIndex;
    std::string prevLogTerm;
    std::string leaderCommit;

    std::getline(
        stream,
        term,
        ','
    );

    std::getline(
        stream,
        leaderId,
        ','
    );

    std::getline(
        stream,
        prevLogIndex,
        ','
    );

    std::getline(
        stream,
        prevLogTerm,
        ','
    );

    std::getline(
        stream,
        leaderCommit,
        ','
    );

    return {
        std::stoi(term),
        std::stoi(leaderId),
        std::stoi(prevLogIndex),
        std::stoi(prevLogTerm),
        std::stoi(leaderCommit)
    };
}

AppendEntriesResponse
RaftNetwork::decodeAppendEntriesResponse(
    const RpcMessage& message
) const {
    std::stringstream stream(
        message.payload
    );

    std::string term;
    std::string success;

    std::getline(
        stream,
        term,
        ','
    );

    std::getline(
        stream,
        success,
        ','
    );

    return {
        std::stoi(term),
        success == "1"
    };
}