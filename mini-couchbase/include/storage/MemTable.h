#pragma once

#include <string>
#include <unordered_map>
#include <cstddef>

class MemTable {
public:
    MemTable(std::size_t maxEntries);

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

    std::size_t size() const;

    bool isFull() const;

    const std::unordered_map<std::string, std::string>&
    getData() const;

private:
    std::unordered_map<std::string, std::string> data;
    std::size_t maxEntries;
};