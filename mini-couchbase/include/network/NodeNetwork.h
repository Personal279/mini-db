#pragma once

#include <functional>
#include <optional>
#include <string>
#include <mutex>
#include <unordered_map>

#include "network/RpcMessage.h"

class NodeNetwork {
public:
    using MessageHandler =
        std::function<std::optional<RpcMessage>(
            const RpcMessage&
        )>;

    NodeNetwork(
        const std::string& host,
        int port
    );

    void start();

    std::optional<RpcMessage> send(
        const std::string& host,
        int port,
        const RpcMessage& message
    );

    void setMessageHandler(
        int partitionId,
        MessageHandler handler
    );

private:
    std::string host;
    int port;

    std::unordered_map<int, MessageHandler> messageHandlers;
    std::mutex handlersMutex;

    void handleConnection(
        int clientSocket
    );
};