#pragma once

#include <string>

#include "network/RpcMessage.h"

class RpcCodec {
public:
    static std::string encode(
        const RpcMessage& message
    );

    static RpcMessage decode(
        const std::string& data
    );

private:
    static std::string typeToString(
        RpcType type
    );

    static RpcType stringToType(
        const std::string& type
    );
};