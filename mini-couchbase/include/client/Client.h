#pragma once

#include <string>

#include "common/Types.h"

class Client {
public:
    Request createGetRequest(const std::string& key) const;

    Request createPutRequest(
        const std::string& key,
        const std::string& value
    ) const;

    Request createDeleteRequest(const std::string& key) const;
};