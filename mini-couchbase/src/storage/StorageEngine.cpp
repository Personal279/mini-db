#include "storage/StorageEngine.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>


void StorageEngine::discoverSSTables() {

    std::vector<
        std::pair<std::size_t, std::string>
    > discoveredFiles;


    for (
        const auto& entry :
        std::filesystem::directory_iterator(
            dataDirectory
        )
    ) {

        if (!entry.is_regular_file()) {
            continue;
        }


        std::string filename =
            entry.path().filename().string();


        const std::string prefix =
            "sstable_";

        const std::string suffix =
            ".db";


        if (
            filename.rfind(prefix, 0) != 0
        ) {
            continue;
        }


        if (
            filename.size() <=
            prefix.size() + suffix.size()
        ) {
            continue;
        }


        if (
            filename.substr(
                filename.size() - suffix.size()
            ) != suffix
        ) {
            continue;
        }


        std::string numberString =
            filename.substr(
                prefix.size(),
                filename.size()
                    - prefix.size()
                    - suffix.size()
            );


        try {

            std::size_t number =
                std::stoull(numberString);


            discoveredFiles.push_back(
                {
                    number,
                    entry.path().string()
                }
            );
        }
        catch (...) {

            continue;
        }
    }


    std::sort(
        discoveredFiles.begin(),
        discoveredFiles.end(),
        [](
            const auto& a,
            const auto& b
        ) {
            return a.first < b.first;
        }
    );


    for (
        const auto& file :
        discoveredFiles
    ) {

        sstableFiles.push_back(
            std::make_shared<SSTableFile>(
                file.second
            )
        );
    }


    if (!discoveredFiles.empty()) {

        sstableCounter.store(
            discoveredFiles.back().first + 1
        );
    }
}


StorageEngine::StorageEngine(
    const std::string& walPath,
    const std::string& dataDirectory,
    std::size_t memTableMaxEntries
)
    : wal(walPath),
      dataDirectory(dataDirectory),
      memTableMaxEntries(memTableMaxEntries),
      activeMemTable(
          std::make_unique<MemTable>(
              memTableMaxEntries
          )
      ) {

    std::filesystem::create_directories(
        dataDirectory
    );


    discoverSSTables();


    std::vector<Request> requests =
        wal.recover();


    for (
        const Request& request :
        requests
    ) {

        switch (
            request.operation
        ) {

            case Operation::PUT:

                activeMemTable->put(
                    request.key,
                    request.value
                );


                if (
                    activeMemTable->isFull()
                ) {

                    rotateMemTable();
                }

                break;


            case Operation::DELETE:

                activeMemTable->remove(
                    request.key
                );


                if (
                    activeMemTable->isFull()
                ) {

                    rotateMemTable();
                }

                break;


            case Operation::GET:
                break;
        }
    }


    compactionThread =
        std::thread(
            &StorageEngine::compactionWorker,
            this
        );
}


StorageEngine::~StorageEngine() {

    {
        std::lock_guard<std::mutex>
            lock(compactionMutex);

        shutdownCompaction = true;

        compactionRequested = true;
    }


    compactionCondition.notify_one();


    if (
        compactionThread.joinable()
    ) {

        compactionThread.join();
    }


    {
        std::lock_guard<std::mutex>
            lock(flushMutex);


        for (
            auto& task :
            flushTasks
        ) {

            task.wait();
        }
    }


    cleanupFinishedFlushTasks();


    cleanupRetiredSSTables();
}


void StorageEngine::rotateMemTable() {

    std::unique_ptr<MemTable>
        oldMemTable =
            std::move(activeMemTable);


    activeMemTable =
        std::make_unique<MemTable>(
            memTableMaxEntries
        );


    flushMemTableAsync(
        std::move(oldMemTable)
    );


    cleanupFinishedFlushTasks();
}


void StorageEngine::flushMemTableAsync(
    std::unique_ptr<MemTable> memTable
) {

    std::size_t tableNumber =
        sstableCounter.fetch_add(1);


    std::string filePath =
        dataDirectory +
        "/sstable_" +
        std::to_string(tableNumber) +
        ".db";


    auto task =
        std::async(
            std::launch::async,

            [
                this,
                memTable =
                    std::move(memTable),
                filePath
            ]() mutable {

                bool success =
                    SSTable::write(
                        *memTable,
                        filePath
                    );


                if (!success) {
                    return;
                }


                bool shouldCompact =
                    false;


                {
                    std::lock_guard<std::mutex>
                        lock(sstableMutex);


                    sstableFiles.push_back(
                        std::make_shared<SSTableFile>(
                            filePath
                        )
                    );


                    if (
                        sstableFiles.size() >=
                        COMPACTION_THRESHOLD
                    ) {

                        shouldCompact = true;
                    }
                }


                if (shouldCompact) {

                    requestCompaction();
                }
            }
        );


    {
        std::lock_guard<std::mutex>
            lock(flushMutex);


        flushTasks.push_back(
            std::move(task)
        );
    }
}


void StorageEngine::cleanupFinishedFlushTasks() {

    std::lock_guard<std::mutex>
        lock(flushMutex);


    for (
        auto it =
            flushTasks.begin();

        it != flushTasks.end();
    ) {

        if (
            it->wait_for(
                std::chrono::seconds(0)
            ) ==
            std::future_status::ready
        ) {

            it->get();

            it =
                flushTasks.erase(it);
        }
        else {

            ++it;
        }
    }
}


void StorageEngine::requestCompaction() {

    {
        std::lock_guard<std::mutex>
            lock(compactionMutex);

        compactionRequested = true;
    }


    compactionCondition.notify_one();
}


void StorageEngine::compactionWorker() {

    while (true) {

        {
            std::unique_lock<std::mutex>
                lock(compactionMutex);


            compactionCondition.wait(
                lock,

                [this] {

                    return
                        compactionRequested ||
                        shutdownCompaction;
                }
            );


            if (
                shutdownCompaction
            ) {

                break;
            }


            compactionRequested = false;
        }


        compact();


        cleanupRetiredSSTables();


        cleanupFinishedFlushTasks();
    }
}


void StorageEngine::put(
    const std::string& key,
    const std::string& value
) {

    Request request{
        Operation::PUT,
        key,
        value
    };


    wal.append(request);


    activeMemTable->put(
        key,
        value
    );


    if (
        activeMemTable->isFull()
    ) {

        rotateMemTable();
    }
}


std::string StorageEngine::get(
    const std::string& key
) const {

    std::string value =
        activeMemTable->get(key);


    if (
        value ==
        MemTable::TOMBSTONE
    ) {

        return "";
    }


    if (!value.empty()) {

        return value;
    }


    std::vector<
        std::shared_ptr<SSTableFile>
    > snapshot;


    {
        std::lock_guard<std::mutex>
            lock(sstableMutex);

        snapshot = sstableFiles;
    }


//    for (
//        auto it = snapshot.rbegin();
//        it != snapshot.rend();
//        ++it
//    ) {
//
//        value =
//            SSTable::readWithIndex(
//                key,
//                (*it)->filePath,
//                (*it)->index
//            );
//
//
//        if (
//            value ==
//            MemTable::TOMBSTONE
//        ) {
//
//            return "";
//        }
//
//
//        if (!value.empty()) {
//
//            return value;
//        }
//    }


    for (
    auto it = snapshot.rbegin();
    it != snapshot.rend();
    ++it
) {

    // Bloom Filter says the key definitely
    // does not exist in this SSTable.
    if (
        !(*it)->bloomFilter.possiblyContains(key)
    ) {
        continue;
    }

    // Key may exist, so check the index
    // and directly seek to its position.
    value =
        SSTable::readWithIndex(
            key,
            (*it)->filePath,
            (*it)->index
        );

    if (
        value ==
        MemTable::TOMBSTONE
    ) {

        return "";
    }

    if (!value.empty()) {

        return value;
    }
}

    return "";
}


void StorageEngine::remove(
    const std::string& key
) {

    Request request{
        Operation::DELETE,
        key,
        ""
    };


    wal.append(request);


    activeMemTable->remove(
        key
    );


    if (
        activeMemTable->isFull()
    ) {

        rotateMemTable();
    }
}


void StorageEngine::compact() {

    std::vector<
        std::shared_ptr<SSTableFile>
    > tablesToCompact;


    {
        std::lock_guard<std::mutex>
            lock(sstableMutex);


        if (
            sstableFiles.size() <
            COMPACTION_THRESHOLD
        ) {

            return;
        }


        tablesToCompact =
            sstableFiles;
    }


    std::unordered_map<
        std::string,
        std::string
    > mergedData;


    for (
        const auto& table :
        tablesToCompact
    ) {

        std::ifstream file(
            table->filePath
        );


        if (!file.is_open()) {
            continue;
        }


        std::string line;


        while (
            std::getline(file, line)
        ) {

            std::size_t separator =
                line.find('|');


            if (
                separator ==
                std::string::npos
            ) {

                continue;
            }


            std::string key =
                line.substr(
                    0,
                    separator
                );


            std::string value =
                line.substr(
                    separator + 1
                );


            // Newer SSTables overwrite older values
            mergedData[key] = value;
        }
    }


    for (
        auto it =
            mergedData.begin();

        it != mergedData.end();
    ) {

        if (
            it->second ==
            MemTable::TOMBSTONE
        ) {

            it =
                mergedData.erase(it);
        }
        else {

            ++it;
        }
    }


    std::size_t tableNumber =
        sstableCounter.fetch_add(1);


    std::string compactedFilePath =
        dataDirectory +
        "/sstable_" +
        std::to_string(tableNumber) +
        ".db";


    std::ofstream output(
        compactedFilePath
    );


    if (!output.is_open()) {

        return;
    }


    for (
        const auto& entry :
        mergedData
    ) {

        output
            << entry.first
            << "|"
            << entry.second
            << "\n";
    }


    output.close();


    auto newTable =
        std::make_shared<SSTableFile>(
            compactedFilePath
        );


    {
        std::lock_guard<std::mutex>
            lock(sstableMutex);


        std::vector<
            std::shared_ptr<SSTableFile>
        > newActiveTables;


        for (
            const auto& table :
            sstableFiles
        ) {

            bool wasCompacted =
                false;


            for (
                const auto& oldTable :
                tablesToCompact
            ) {

                if (
                    table ==
                    oldTable
                ) {

                    wasCompacted = true;

                    break;
                }
            }


            if (wasCompacted) {

                retiredSSTables.push_back(
                    table
                );
            }
            else {

                newActiveTables.push_back(
                    table
                );
            }
        }


        newActiveTables.push_back(
            newTable
        );


        sstableFiles =
            std::move(newActiveTables);
    }
}


void StorageEngine::cleanupRetiredSSTables() {

    std::lock_guard<std::mutex>
        lock(sstableMutex);


    for (
        auto it =
            retiredSSTables.begin();

        it != retiredSSTables.end();
    ) {

        if (
            (*it).use_count() == 1
        ) {

            std::filesystem::remove(
                (*it)->filePath
            );


            it =
                retiredSSTables.erase(it);
        }
        else {

            ++it;
        }
    }
}