#include "storage/StorageEngine.h"

#include <algorithm>
#include <filesystem>
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

        const std::string prefix = "sstable_";
        const std::string suffix = ".db";

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
                filename.size() -
                prefix.size() -
                suffix.size()
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
            file.second
        );
    }

    if (!discoveredFiles.empty()) {

        sstableCounter =
            discoveredFiles.back().first + 1;
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

    // Create directory if it does not exist
    std::filesystem::create_directories(
        dataDirectory
    );

    discoverSSTables();

    std::vector<Request> requests =
        wal.recover();

    for (const Request& request : requests) {

        switch (request.operation) {

            case Operation::PUT:

                activeMemTable->put(
                    request.key,
                    request.value
                );

                if (activeMemTable->isFull()) {
                    rotateMemTable();
                }

                break;


            case Operation::DELETE:

                activeMemTable->remove(
                    request.key
                );

                if (activeMemTable->isFull()) {
                    rotateMemTable();
                }

                break;


            case Operation::GET:
                break;
        }
    }
}


void StorageEngine::rotateMemTable() {

    immutableMemTable =
        std::move(activeMemTable);

    activeMemTable =
        std::make_unique<MemTable>(
            memTableMaxEntries
        );

    flushImmutableMemTable();
}


void StorageEngine::flushImmutableMemTable() {

    if (!immutableMemTable) {
        return;
    }

    std::string filePath =
        dataDirectory +
        "/sstable_" +
        std::to_string(sstableCounter++) +
        ".db";

    bool success = SSTable::write(
        *immutableMemTable,
        filePath
    );

    if (success) {

        sstableFiles.push_back(
            filePath
        );

        immutableMemTable.reset();
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

    if (activeMemTable->isFull()) {
        rotateMemTable();
    }
}


std::string StorageEngine::get(
    const std::string& key
) const {

    std::string value =
        activeMemTable->get(key);

    if (value == MemTable::TOMBSTONE) {
        return "";
    }

    if (!value.empty()) {
        return value;
    }


    if (immutableMemTable) {

        value =
            immutableMemTable->get(key);

        if (value == MemTable::TOMBSTONE) {
            return "";
        }

        if (!value.empty()) {
            return value;
        }
    }


    for (
        auto it = sstableFiles.rbegin();
        it != sstableFiles.rend();
        ++it
    ) {

        value = SSTable::read(
            key,
            *it
        );

        if (value == MemTable::TOMBSTONE) {
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

    if (activeMemTable->isFull()) {
        rotateMemTable();
    }
}