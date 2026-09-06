#include "storage/StorageEngine.h"

StorageEngine::StorageEngine(
    const std::string& walPath
)
    : wal(walPath) {

    std::vector<Request> requests = wal.recover();

    for (const Request& request : requests) {

        switch (request.operation) {

            case Operation::PUT:
                data[request.key] = request.value;
                break;

            case Operation::DELETE:
                data.erase(request.key);
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

    data[key] = value;
}


std::string StorageEngine::get(
    const std::string& key
) const {
    auto it = data.find(key);

    if (it == data.end()) {
        return "";
    }

    return it->second;
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

    data.erase(key);
}