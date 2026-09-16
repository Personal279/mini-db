#include "raft/RaftNetwork.h"

#include <sstream>
#include <stdexcept>
#include <utility>

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

void RaftNetwork::setHeartbeatCallback(
    std::function<void()> callback
) {
    heartbeatCallback =
        std::move(callback);
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

    RpcMessage message{
        RpcType::APPEND_ENTRIES,
        nodeId,
        -1,
        encodeAppendEntries(request)
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

        RequestVoteHandler handler(group);

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

        AppendEntriesHandler handler(group);

        AppendEntriesResponse response =
            handler.handle(request);

        if (
            response.success &&
            heartbeatCallback
        ) {

            heartbeatCallback();
        }

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

std::string RaftNetwork::encodeAppendEntries(
    const AppendEntriesRequest& request
) const {

    std::stringstream stream;

    stream
        << request.term << "|"
        << request.leaderId << "|"
        << request.prevLogIndex << "|"
        << request.prevLogTerm << "|"
        << request.leaderCommit << "|"
        << request.entries.size();

    for (
        const LogEntry& entry :
        request.entries
    ) {

        stream
            << "|"
            << entry.term
            << "|"
            << entry.index
            << "|"
            << entry.command;
    }

    return stream.str();
}

AppendEntriesRequest
RaftNetwork::decodeAppendEntries(
    const RpcMessage& message
) const {

    std::stringstream stream(
        message.payload
    );

    std::string value;

    std::getline(
        stream,
        value,
        '|'
    );

    int term =
        std::stoi(value);

    std::getline(
        stream,
        value,
        '|'
    );

    int leaderId =
        std::stoi(value);

    std::getline(
        stream,
        value,
        '|'
    );

    int prevLogIndex =
        std::stoi(value);

    std::getline(
        stream,
        value,
        '|'
    );

    int prevLogTerm =
        std::stoi(value);

    std::getline(
        stream,
        value,
        '|'
    );

    int leaderCommit =
        std::stoi(value);

    std::getline(
        stream,
        value,
        '|'
    );

    int entryCount =
        std::stoi(value);

    std::vector<LogEntry> entries;

    for (
        int i = 0;
        i < entryCount;
        i++
    ) {

        std::string entryTerm;
        std::string entryIndex;
        std::string command;

        std::getline(
            stream,
            entryTerm,
            '|'
        );

        std::getline(
            stream,
            entryIndex,
            '|'
        );

        if (
            i + 1 ==
            entryCount
        ) {

            std::getline(
                stream,
                command
            );

        } else {

            std::getline(
                stream,
                command,
                '|'
            );
        }

        entries.push_back({
            std::stoi(entryTerm),
            std::stoi(entryIndex),
            command
        });
    }

    return {
        term,
        leaderId,
        prevLogIndex,
        prevLogTerm,
        entries,
        leaderCommit
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