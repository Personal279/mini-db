#include "raft/RaftNetwork.h"

#include <sstream>
#include <stdexcept>

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
        encodeAppendEntries(request);

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

std::string RaftNetwork::encodeAppendEntries(
    const AppendEntriesRequest& request
) const {
    return
        std::to_string(request.term) + "|" +
        std::to_string(request.leaderId) + "|" +
        std::to_string(request.prevLogIndex) + "|" +
        std::to_string(request.prevLogTerm) + "|" +
        std::to_string(request.leaderCommit) + "|" +
        encodeLogEntries(request.entries);
}

std::string RaftNetwork::encodeLogEntries(
    const std::vector<LogEntry>& entries
) const {
    std::string result;

    for (
        std::size_t i = 0;
        i < entries.size();
        ++i
    ) {
        const LogEntry& entry =
            entries[i];

        result +=
            std::to_string(entry.term) + "," +
            std::to_string(entry.index) + "," +
            entry.command;

        if (
            i + 1 <
            entries.size()
        ) {
            result += ";";
        }
    }

    return result;
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
    std::string entriesData;

    std::getline(
        stream,
        term,
        '|'
    );

    std::getline(
        stream,
        leaderId,
        '|'
    );

    std::getline(
        stream,
        prevLogIndex,
        '|'
    );

    std::getline(
        stream,
        prevLogTerm,
        '|'
    );

    std::getline(
        stream,
        leaderCommit,
        '|'
    );

    std::getline(
        stream,
        entriesData
    );

    return {
        std::stoi(term),
        std::stoi(leaderId),
        std::stoi(prevLogIndex),
        std::stoi(prevLogTerm),
        decodeLogEntries(entriesData),
        std::stoi(leaderCommit)
    };
}

std::vector<LogEntry>
RaftNetwork::decodeLogEntries(
    const std::string& data
) const {
    std::vector<LogEntry> entries;

    if (data.empty()) {
        return entries;
    }

    std::stringstream entryStream(
        data
    );

    std::string entryData;

    while (
        std::getline(
            entryStream,
            entryData,
            ';'
        )
    ) {
        std::stringstream stream(
            entryData
        );

        std::string term;
        std::string index;
        std::string command;

        std::getline(
            stream,
            term,
            ','
        );

        std::getline(
            stream,
            index,
            ','
        );

        std::getline(
            stream,
            command
        );

        if (
            term.empty() ||
            index.empty()
        ) {
            throw std::runtime_error(
                "Invalid log entry"
            );
        }

        entries.push_back({
            std::stoi(term),
            std::stoi(index),
            command
        });
    }

    return entries;
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