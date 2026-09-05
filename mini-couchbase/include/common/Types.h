#pragma once

#include <string>

enum class Operation {
    GET,
    PUT,
    DELETE
};

struct Request {
    Operation operation;
    std::string key;
    std::string value;
};