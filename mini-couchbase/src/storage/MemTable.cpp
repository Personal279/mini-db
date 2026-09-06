#include "storage/MemTable.h"


void MemTable::put(
    const std::string& key,
    const std::string& value
) {
    data[key] = value;
}


std::string MemTable::get(
    const std::string& key
) const {
    auto it = data.find(key);

    if (it == data.end()) {
        return "";
    }

    return it->second;
}


void MemTable::remove(
    const std::string& key
) {
    data.erase(key);
}