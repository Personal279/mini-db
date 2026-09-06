#pragma once

#include <string>

#include "storage/WAL.h"
#include "storage/MemTable.h"

class StorageEngine {
public:
    StorageEngine(const std::string& walPath);

    void put(
        const std::string& key,
        const std::string& value
    );

    std::string get(
        const std::string& key
    ) const;

    void remove(
        const std::string& key
    );

private:
    WAL wal;
    MemTable memTable;
};