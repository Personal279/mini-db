#include "storage/StorageEngine.h"

void StorageEngine::put(
    const std::string& key,
    const std::string& value
) {
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
    data.erase(key);
}