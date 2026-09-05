#pragma once

#include <string>

#include "common/Types.h"

class WAL {
public:
    WAL(const std::string& filePath);

    void append(const Request& request);

private:
    std::string filePath;
};