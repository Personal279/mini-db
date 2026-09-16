#pragma once
#include <vector>
#include <functional>
#include "node/RequestServer.h"

class HttpServer {
public:
    explicit HttpServer(std::vector<std::reference_wrapper<RequestServer>> servers);
    void start();

private:
    std::vector<std::reference_wrapper<RequestServer>> servers;
    Response dispatch(const Request& request);
};