#pragma once

#include <string>

enum class RpcType {
    REQUEST_VOTE,
    VOTE_RESPONSE,
    APPEND_ENTRIES,
    APPEND_ENTRIES_RESPONSE,
    CLIENT_REQUEST,
    CLIENT_RESPONSE,
    NOT_LEADER
};

struct RpcMessage {
    RpcType type;
    int senderId;
    int receiverId;
    std::string payload;
};