#pragma once

#include <string>
#include <vector>

#include "common/Types.h"

class WAL {
public:
    WAL(const std::string& filePath);

    void append(const Request& request);

    std::vector<Request> recover();

private:
    std::string filePath;
};