#pragma once

#include <string>
#include <unordered_map>

#include "common/Types.h"
#include "storage/WAL.h"

class StorageEngine {
public:
    StorageEngine(const std::string& walPath);

    void put(
        const std::string& key,
        const std::string& value
    );

    std::string get(const std::string& key) const;

    void remove(const std::string& key);

private:
    std::unordered_map<std::string, std::string> data;

    WAL wal;
};