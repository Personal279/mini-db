#pragma once

#include <string>

#include "storage/MemTable.h"

class SSTable {
public:
    static bool write(
        const MemTable& memTable,
        const std::string& filePath
    );

    static std::string read(
        const std::string& key,
        const std::string& filePath
    );
};