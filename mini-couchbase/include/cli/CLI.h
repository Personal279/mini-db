#pragma once

#include "client/Client.h"
#include "node/RequestServer.h"
#include "router/RoutingService.h"

class CLI {
public:
    CLI(
        Client& client,
        RoutingService& routingService,
        RequestServer& serverA,
        RequestServer& serverB,
        RequestServer& serverC
    );

    void run();

private:
    Client& client;
    RoutingService& routingService;

    RequestServer& serverA;
    RequestServer& serverB;
    RequestServer& serverC;

    void handleCommand(
        const std::string& command
    );
};