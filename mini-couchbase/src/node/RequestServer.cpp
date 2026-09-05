#include "node/RequestServer.h"

#include <iostream>

void RequestServer::handleRequest(const Request& request) {
    std::cout << "Request received: ";

    switch (request.operation) {
        case Operation::GET:
            std::cout << "GET";
            break;

        case Operation::PUT:
            std::cout << "PUT";
            break;

        case Operation::DELETE:
            std::cout << "DELETE";
            break;
    }

    std::cout << " | Key: " << request.key;

    if (request.operation == Operation::PUT) {
        std::cout << " | Value: " << request.value;
    }

    std::cout << std::endl;
}