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

    // Recover previous operations from WAL
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

    // Check active MemTable first
    std::string value = activeMemTable->get(key);

    if (!value.empty()) {
        return value;
    }

    // Then check immutable MemTable
    if (immutableMemTable) {
        value = immutableMemTable->get(key);
    }

    return value;
}


void StorageEngine::remove(
    const std::string& key
) {
    Request request{
        Operation::DELETE,
        key,
        ""
    };

    // Write to WAL first
    wal.append(request);

    // Remove from active MemTable
    activeMemTable->remove(key);

    // Also remove from immutable MemTable if it exists
    if (immutableMemTable) {
        immutableMemTable->remove(key);
    }
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
        immutableMemTable.reset();
    }
}