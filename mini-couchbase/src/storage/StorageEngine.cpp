#include "storage/StorageEngine.h"

#include <vector>


StorageEngine::StorageEngine(
    const std::string& walPath,
    std::size_t memTableMaxEntries
)
    : wal(walPath),
      memTableMaxEntries(memTableMaxEntries),
      activeMemTable(
          std::make_unique<MemTable>(memTableMaxEntries)
      ) {

    std::vector<Request> requests = wal.recover();

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
                activeMemTable->remove(request.key);
                break;

            case Operation::GET:
                break;
        }
    }
}


void StorageEngine::rotateMemTable() {

    immutableMemTable = std::move(activeMemTable);

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
        "sstable_" +
        std::to_string(sstableCounter++) +
        ".db";

    bool success = SSTable::write(
        *immutableMemTable,
        filePath
    );

    if (success) {

        sstableFiles.push_back(filePath);

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

    activeMemTable->put(key, value);

    if (activeMemTable->isFull()) {
        rotateMemTable();
    }
}


std::string StorageEngine::get(
    const std::string& key
) const {

    std::string value =
        activeMemTable->get(key);

    if (!value.empty()) {
        return value;
    }

    if (immutableMemTable) {

        value =
            immutableMemTable->get(key);

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

    activeMemTable->remove(key);

    if (immutableMemTable) {
        immutableMemTable->remove(key);
    }
}