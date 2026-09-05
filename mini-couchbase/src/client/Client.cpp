#include "client/Client.h"

Request Client::createGetRequest(const std::string& key) const {
    return {
        Operation::GET,
        key,
        ""
    };
}

Request Client::createPutRequest(
    const std::string& key,
    const std::string& value
) const {
    return {
        Operation::PUT,
        key,
        value
    };
}

Request Client::createDeleteRequest(const std::string& key) const {
    return {
        Operation::DELETE,
        key,
        ""
    };
}