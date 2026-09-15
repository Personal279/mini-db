#pragma once

#include <optional>
#include <string>

#include "network/NodeNetwork.h"
#include "raft/AppendEntries.h"
#include "raft/RequestVote.h"

class RaftNetwork {
public:
    RaftNetwork(
        NodeNetwork& network,
        int nodeId,
        RaftGroup& group
    );

    std::optional<RequestVoteResponse> sendRequestVote(
        const std::string& host,
        int port,
        const RequestVoteRequest& request
    );

    std::optional<AppendEntriesResponse> sendAppendEntries(
        const std::string& host,
        int port,
        const AppendEntriesRequest& request
    );

private:
    NodeNetwork& network;
    int nodeId;
    RaftGroup& group;

    std::optional<RpcMessage> handleMessage(
        const RpcMessage& message
    );

    RequestVoteRequest decodeRequestVote(
        const RpcMessage& message
    ) const;

    RequestVoteResponse decodeVoteResponse(
        const RpcMessage& message
    ) const;

    AppendEntriesRequest decodeAppendEntries(
        const RpcMessage& message
    ) const;

    AppendEntriesResponse decodeAppendEntriesResponse(
        const RpcMessage& message
    ) const;

    std::string encodeAppendEntries(
        const AppendEntriesRequest& request
    ) const;

    std::string encodeLogEntries(
        const std::vector<LogEntry>& entries
    ) const;

    std::vector<LogEntry> decodeLogEntries(
        const std::string& data
    ) const;
};