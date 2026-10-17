#include "network/RpcCodec.h"

#include <sstream>
#include <stdexcept>
std::string RpcCodec::encode(const RpcMessage& message) {
    return
        typeToString(message.type) + "|" +
        std::to_string(message.senderId) + "|" +
        std::to_string(message.receiverId) + "|" +
        std::to_string(message.partitionId) + "|" +
        message.payload +
        "\n";
}

RpcMessage RpcCodec::decode(const std::string& data) {
    std::stringstream stream(data);

    std::string type;
    std::string senderId;
    std::string receiverId;
    std::string partitionId;
    std::string payload;

    std::getline(stream, type, '|');
    std::getline(stream, senderId, '|');
    std::getline(stream, receiverId, '|');
    std::getline(stream, partitionId, '|');
    std::getline(stream, payload);

    if (type.empty() ||
        senderId.empty() ||
        receiverId.empty() ||
        partitionId.empty()) {
        throw std::runtime_error("Invalid RPC message");
    }

    return {
        stringToType(type),
        std::stoi(senderId),
        std::stoi(receiverId),
        payload,
        std::stoi(partitionId)
    };
}


std::string RpcCodec::typeToString(
    RpcType type
) {
    switch (type) {
        case RpcType::REQUEST_VOTE:
            return "REQUEST_VOTE";

        case RpcType::VOTE_RESPONSE:
            return "VOTE_RESPONSE";

        case RpcType::APPEND_ENTRIES:
            return "APPEND_ENTRIES";

        case RpcType::APPEND_ENTRIES_RESPONSE:
            return "APPEND_ENTRIES_RESPONSE";

        case RpcType::CLIENT_REQUEST:
            return "CLIENT_REQUEST";

        case RpcType::CLIENT_RESPONSE:
            return "CLIENT_RESPONSE";

        case RpcType::NOT_LEADER:
            return "NOT_LEADER";
    }

    throw std::runtime_error(
        "Unknown RPC type"
    );
}

RpcType RpcCodec::stringToType(
    const std::string& type
) {
    if (type == "REQUEST_VOTE")
        return RpcType::REQUEST_VOTE;

    if (type == "VOTE_RESPONSE")
        return RpcType::VOTE_RESPONSE;

    if (type == "APPEND_ENTRIES")
        return RpcType::APPEND_ENTRIES;

    if (type == "APPEND_ENTRIES_RESPONSE")
        return RpcType::APPEND_ENTRIES_RESPONSE;

    if (type == "CLIENT_REQUEST")
        return RpcType::CLIENT_REQUEST;

    if (type == "CLIENT_RESPONSE")
        return RpcType::CLIENT_RESPONSE;

    if (type == "NOT_LEADER")
        return RpcType::NOT_LEADER;

    throw std::runtime_error(
        "Unknown RPC type: " + type
    );
}