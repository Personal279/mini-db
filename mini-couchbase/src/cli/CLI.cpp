#include "cli/CLI.h"

#include <iostream>
#include <sstream>

CLI::CLI(
    Client& client,
    RoutingService& routingService,
    RequestServer& serverA,
    RequestServer& serverB,
    RequestServer& serverC
)
    : client(client),
      routingService(routingService),
      serverA(serverA),
      serverB(serverB),
      serverC(serverC) {
}

void CLI::run() {
    std::cout << "MDDB CLI started\n";
    std::cout << "Type HELP for commands\n\n";

    std::string command;

    while (true) {
        std::cout << "mddb> ";

        std::getline(
            std::cin,
            command
        );

        if (command == "EXIT") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (command == "HELP") {
            std::cout << "\nCommands:\n";
            std::cout << "PUT <key> <value>\n";
            std::cout << "GET <key>\n";
            std::cout << "DELETE <key>\n";
            std::cout << "EXIT\n\n";
            continue;
        }

        handleCommand(command);
    }
}

void CLI::handleCommand(
    const std::string& command
) {
    std::stringstream stream(command);

    std::string operation;
    std::string key;
    std::string value;

    stream >> operation;

    Request request;

    if (operation == "PUT") {
        stream >> key;
        stream >> value;

        if (key.empty() || value.empty()) {
            std::cout
                << "Usage: PUT <key> <value>\n";
            return;
        }

        request = client.createPutRequest(
            key,
            value
        );
    }
    else if (operation == "GET") {
        stream >> key;

        if (key.empty()) {
            std::cout
                << "Usage: GET <key>\n";
            return;
        }

        request = client.createGetRequest(key);
    }
    else if (operation == "DELETE") {
        stream >> key;

        if (key.empty()) {
            std::cout
                << "Usage: DELETE <key>\n";
            return;
        }

        request = client.createDeleteRequest(key);
    }
    else {
        std::cout
            << "Unknown command. Type HELP.\n";
        return;
    }

    int nodeId =
        routingService.route(request);

    if (nodeId == 0) {
        serverA.handleRequest(request);
    }
    else if (nodeId == 1) {
        serverB.handleRequest(request);
    }
    else if (nodeId == 2) {
        serverC.handleRequest(request);
    }
    else {
        std::cout
            << "Invalid node ID: "
            << nodeId
            << "\n";
    }
}