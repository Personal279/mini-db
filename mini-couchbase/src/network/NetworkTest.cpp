#include <iostream>

#include "network/NodeNetwork.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout
            << "Usage:\n"
            << "  network_test server <nodeId> <port>\n"
            << "  network_test send <senderId> <receiverId> <port>\n";

        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        int nodeId = std::stoi(argv[2]);
        int port = std::stoi(argv[3]);

        NodeNetwork network(
            "127.0.0.1",
            port
        );

        network.start();

        return nodeId;
    }

    if (mode == "send") {
        int senderId = std::stoi(argv[2]);
        int receiverId = std::stoi(argv[3]);
        int port = std::stoi(argv[4]);

        RpcMessage message{
            RpcType::REQUEST_VOTE,
            senderId,
            receiverId,
            "term=5"
        };

        NodeNetwork network(
            "127.0.0.1",
            0
        );

        network.send(
            "127.0.0.1",
            port,
            message
        );

        return 0;
    }

    return 1;
}