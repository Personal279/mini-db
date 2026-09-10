#pragma once

#include <optional>
#include <string>

#include "network/NodeNetwork.h"
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
};