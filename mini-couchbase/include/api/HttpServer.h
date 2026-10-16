#pragma once

#include <functional>
#include <vector>

#include "node/RequestServer.h"
#include "router/RoutingService.h"
#include "cluster/Node.h"

class HttpServer {
public:
    HttpServer(
        std::vector<std::reference_wrapper<RequestServer>> servers,
        RoutingService& routingService,
        std::vector<Node> nodes
    );

    void start();

private:
    std::vector<std::reference_wrapper<RequestServer>> servers;
    RoutingService& routingService;
    std::vector<Node> nodes;

    Response dispatch(const Request& request);
};