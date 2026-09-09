#pragma once

#include <string>
#include <cstddef>
#include <memory>
#include <vector>

#include "storage/WAL.h"
#include "storage/MemTable.h"
#include "storage/SSTable.h"


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

    std::unique_ptr<MemTable> activeMemTable;

    std::unique_ptr<MemTable> immutableMemTable;

    std::vector<std::string> sstableFiles;

    std::size_t sstableCounter = 0;

    void discoverSSTables();

    void rotateMemTable();

    void flushImmutableMemTable();
};