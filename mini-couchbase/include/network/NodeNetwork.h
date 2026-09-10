#pragma once

#include <functional>
#include <optional>
#include <string>

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
        MessageHandler handler
    );

private:
    std::string host;
    int port;

    MessageHandler messageHandler;

    void handleConnection(
        int clientSocket
    );
};