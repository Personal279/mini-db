#pragma once

#include <string>
#include <cstddef>
#include <memory>

#include "storage/WAL.h"
#include "storage/MemTable.h"

class StorageEngine {
public:
    StorageEngine(
        const std::string& walPath,
        std::size_t memTableMaxEntries
    );

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

    std::size_t memTableMaxEntries;

    // Active MemTable receives new writes
    std::unique_ptr<MemTable> activeMemTable;

    // Full MemTable waiting for SSTable flush
    std::unique_ptr<MemTable> immutableMemTable;

    void rotateMemTable();
};