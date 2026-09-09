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

#include "storage/WAL.h"
#include "storage/MemTable.h"
#include "storage/SSTable.h"
#include "storage/BloomFilter.h"


struct SSTableFile {

    explicit SSTableFile(
        const std::string& path
    )
        : filePath(path),
          index(
              SSTable::buildIndex(
                  path
              )
          ),
          bloomFilter(
              SSTable::buildBloomFilter(
                  path
              )
          ) {
    }


    std::string filePath;


    SSTable::Index index;


    BloomFilter bloomFilter;
};


class StorageEngine {

public:

    StorageEngine(
        const std::string& walPath,
        const std::string& dataDirectory,
        std::size_t memTableMaxEntries
    );


    ~StorageEngine();


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

    static constexpr std::size_t
        COMPACTION_THRESHOLD = 4;


    WAL wal;

    std::string dataDirectory;

    std::size_t memTableMaxEntries;


    std::unique_ptr<MemTable>
        activeMemTable;


    // Protects active and retired SSTable lists
    mutable std::mutex
        sstableMutex;


    // Currently active SSTables
    std::vector<
        std::shared_ptr<SSTableFile>
    > sstableFiles;


    // Old SSTables waiting for active GET
    // operations to finish before deletion
    std::vector<
        std::shared_ptr<SSTableFile>
    > retiredSSTables;


    // Thread-safe SSTable numbering
    std::atomic<std::size_t>
        sstableCounter{0};


    // Async flush tasks
    std::mutex
        flushMutex;


    std::vector<
        std::future<void>
    > flushTasks;


    // Background compaction worker
    std::thread
        compactionThread;


    std::mutex
        compactionMutex;


    std::condition_variable
        compactionCondition;


    bool compactionRequested = false;

    bool shutdownCompaction = false;


    void discoverSSTables();


    void rotateMemTable();


    void flushMemTableAsync(
        std::unique_ptr<MemTable> memTable
    );


    void cleanupFinishedFlushTasks();


    void requestCompaction();


    void compactionWorker();


    void compact();


    void cleanupRetiredSSTables();
};