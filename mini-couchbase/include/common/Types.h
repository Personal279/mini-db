#pragma once

#include <string>

enum class Operation {
    PUT,
    DELETE,
    GET
};

struct Request {
    Operation operation;
    std::string key;
    std::string value;
};

struct Response {
    bool success;
    bool isLeader;
    std::string value;
    std::string message;
};