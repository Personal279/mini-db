#include "storage/StorageEngine.h"

#include <vector>


StorageEngine::StorageEngine(
    const std::string& walPath,
    std::size_t memTableMaxEntries
)
    : wal(walPath),
      memTable(memTableMaxEntries) {

    std::vector<Request> requests = wal.recover();

    for (const Request& request : requests) {

        switch (request.operation) {

            case Operation::PUT:
                memTable.put(
                    request.key,
                    request.value
                );
                break;

            case Operation::DELETE:
                memTable.remove(request.key);
                break;

            case Operation::GET:
                break;
        }
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

    memTable.put(key, value);
}


std::string StorageEngine::get(
    const std::string& key
) const {
    return memTable.get(key);
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

    memTable.remove(key);
}