#pragma once

#include "client/Client.h"
#include "router/RequestRouter.h"
#include "node/RequestServer.h"

class CLI {
public:

    CLI(
        Client& client,
        RequestRouter& router,
        RequestServer& serverA,
        RequestServer& serverB,
        RequestServer& serverC
    );

    void run();

private:

    Client& client;
    RequestRouter& router;

    RequestServer& serverA;
    RequestServer& serverB;
    RequestServer& serverC;

    void handleCommand(
        const std::string& command
    );
};