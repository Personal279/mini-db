#include "network/NodeNetwork.h"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "network/RpcCodec.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

NodeNetwork::NodeNetwork(
    const std::string& host,
    int port
)
    : host(host),
      port(port) {
}

void NodeNetwork::setMessageHandler(
    MessageHandler handler
) {
    messageHandler = std::move(handler);
}

void NodeNetwork::start() {

#ifdef _WIN32

    WSADATA wsaData;

    if (WSAStartup(
            MAKEWORD(2, 2),
            &wsaData
        ) != 0) {

        throw std::runtime_error(
            "WSAStartup failed"
        );
    }

#endif

    int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

#ifdef _WIN32

    if (serverSocket == INVALID_SOCKET) {

#else

    if (serverSocket < 0) {

#endif

        throw std::runtime_error(
            "Failed to create socket"
        );
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_port =
        htons(port);

    if (inet_pton(
            AF_INET,
            host.c_str(),
            &serverAddress.sin_addr
        ) <= 0) {

        throw std::runtime_error(
            "Invalid host address"
        );
    }

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        ) < 0) {

        throw std::runtime_error(
            "Failed to bind socket"
        );
    }

    if (listen(
            serverSocket,
            10
        ) < 0) {

        throw std::runtime_error(
            "Failed to listen on socket"
        );
    }

    std::cout
        << "Node network listening on "
        << host
        << ":"
        << port
        << std::endl;

    while (true) {

        sockaddr_in clientAddress{};

#ifdef _WIN32

        int clientLength =
            sizeof(clientAddress);

        int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &clientAddress
                ),
                &clientLength
            );

#else

        socklen_t clientLength =
            sizeof(clientAddress);

        int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &clientAddress
                ),
                &clientLength
            );

#endif

#ifdef _WIN32

        if (clientSocket == INVALID_SOCKET) {

#else

        if (clientSocket < 0) {

#endif

            continue;
        }

        handleConnection(
            clientSocket
        );
    }
}

std::optional<RpcMessage> NodeNetwork::send(
    const std::string& targetHost,
    int targetPort,
    const RpcMessage& message
) {

#ifdef _WIN32

    WSADATA wsaData;

    if (WSAStartup(
            MAKEWORD(2, 2),
            &wsaData
        ) != 0) {

        throw std::runtime_error(
            "WSAStartup failed"
        );
    }

#endif

    int clientSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

#ifdef _WIN32

    if (clientSocket == INVALID_SOCKET) {

#else

    if (clientSocket < 0) {

#endif

        throw std::runtime_error(
            "Failed to create client socket"
        );
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_port =
        htons(targetPort);

    if (inet_pton(
            AF_INET,
            targetHost.c_str(),
            &serverAddress.sin_addr
        ) <= 0) {

#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif

        throw std::runtime_error(
            "Invalid target host address"
        );
    }

    if (connect(
            clientSocket,
            reinterpret_cast<sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        ) < 0) {

#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif

        throw std::runtime_error(
            "Failed to connect to node"
        );
    }

    std::string encoded =
        RpcCodec::encode(message);

    const char* data =
        encoded.c_str();

    std::size_t totalSent = 0;

    while (totalSent < encoded.size()) {

        int bytesSent =
            ::send(
                clientSocket,
                data + totalSent,
                static_cast<int>(
                    encoded.size() - totalSent
                ),
                0
            );

        if (bytesSent <= 0) {

#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif

            throw std::runtime_error(
                "Failed to send RPC message"
            );
        }

        totalSent +=
            static_cast<std::size_t>(
                bytesSent
            );
    }

    char buffer[4096];

    int bytesReceived =
        recv(
            clientSocket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    std::optional<RpcMessage> response;

    if (bytesReceived > 0) {

        buffer[bytesReceived] =
            '\0';

        response =
            RpcCodec::decode(
                std::string(buffer)
            );
    }

#ifdef _WIN32

    closesocket(clientSocket);

#else

    close(clientSocket);

#endif

    return response;
}

void NodeNetwork::handleConnection(
    int clientSocket
) {
    char buffer[4096];

    int bytesReceived =
        recv(
            clientSocket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    if (bytesReceived <= 0) {

#ifdef _WIN32

        closesocket(clientSocket);

#else

        close(clientSocket);

#endif

        return;
    }

    buffer[bytesReceived] =
        '\0';

    RpcMessage message =
        RpcCodec::decode(
            std::string(buffer)
        );

    std::optional<RpcMessage> response;

    if (messageHandler) {

        response =
            messageHandler(message);
    }

    if (response.has_value()) {

        std::string encoded =
            RpcCodec::encode(
                response.value()
            );

        const char* data =
            encoded.c_str();

        std::size_t totalSent = 0;

        while (
            totalSent < encoded.size()
        ) {

            int bytesSent =
                ::send(
                    clientSocket,
                    data + totalSent,
                    static_cast<int>(
                        encoded.size() - totalSent
                    ),
                    0
                );

            if (bytesSent <= 0) {
                break;
            }

            totalSent +=
                static_cast<std::size_t>(
                    bytesSent
                );
        }
    }

#ifdef _WIN32

    closesocket(clientSocket);

#else

    close(clientSocket);

#endif
}