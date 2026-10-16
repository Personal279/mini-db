
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <unordered_map>

#include "storage/WAL.h"
#include "storage/MemTable.h"
#include "storage/SSTable.h"
#include "storage/BloomFilter.h"

struct SSTableFile {
    explicit SSTableFile(const std::string& path,
                         std::size_t generationNumber)
        : filePath(path),
          generation(generationNumber),
          index(SSTable::buildIndex(path)),
          bloomFilter(SSTable::buildBloomFilter(path)) {}

    std::string filePath;
    std::size_t generation;
    SSTable::Index index;
    BloomFilter bloomFilter;
};

class StorageEngine {
public:
    StorageEngine(const std::string& walPath,
                  const std::string& dataDirectory,
                  std::size_t memTableMaxEntries);

    ~StorageEngine();

    void put(const std::string& key, const std::string& value);
    std::string get(const std::string& key) const;
    void remove(const std::string& key);

    std::vector<std::pair<std::string, std::string>> scanAll() const;

private:
    static constexpr std::size_t COMPACTION_THRESHOLD = 10000000;

    mutable std::mutex storageMutex;

    WAL wal;
    std::string dataDirectory;
    std::size_t memTableMaxEntries;

    std::unique_ptr<MemTable> activeMemTable;

    // MemTables that have been rotated but whose SSTables
    // have not yet become available.
    std::vector<std::shared_ptr<MemTable>> flushingMemTables;

    mutable std::mutex sstableMutex;
    std::vector<std::shared_ptr<SSTableFile>> sstableFiles;
    std::vector<std::shared_ptr<SSTableFile>> retiredSSTables;

    std::atomic<std::size_t> sstableCounter{0};

    std::mutex flushMutex;
    std::vector<std::future<void>> flushTasks;

    std::thread compactionThread;
    std::mutex compactionMutex;
    std::condition_variable compactionCondition;

    bool compactionRequested = false;
    bool shutdownCompaction = false;

    void discoverSSTables();
    void rotateMemTable();

    void flushMemTableAsync(std::shared_ptr<MemTable> memTable);

    void cleanupFinishedFlushTasks();
    void requestCompaction();
    void compactionWorker();
    void compact();
    void cleanupRetiredSSTables();
};
