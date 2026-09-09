#include "storage/SSTable.h"

#include <algorithm>
#include <fstream>
#include <vector>


bool SSTable::write(
    const MemTable& memTable,
    const std::string& filePath
) {

    std::ofstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open()) {
        return false;
    }

    std::vector<
        std::pair<std::string, std::string>
    > entries;

    for (
        const auto& entry :
        memTable.getData()
    ) {

        entries.push_back(entry);
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [](
            const auto& a,
            const auto& b
        ) {
            return a.first < b.first;
        }
    );

    for (
        const auto& entry :
        entries
    ) {

        file
            << entry.first
            << "|"
            << entry.second
            << "\n";
    }

    return true;
}


SSTable::Index SSTable::buildIndex(
    const std::string& filePath
) {

    Index index;

    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open()) {
        return index;
    }

    while (true) {

        std::streampos position =
            file.tellg();

        std::string line;

        if (!std::getline(file, line)) {
            break;
        }

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

        index[key] = position;
    }

    return index;
}


BloomFilter SSTable::buildBloomFilter(
    const std::string& filePath,
    std::size_t filterSize
) {

    BloomFilter bloomFilter(
        filterSize
    );

    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open()) {
        return bloomFilter;
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

        bloomFilter.add(key);
    }

    return bloomFilter;
}


std::string SSTable::readWithIndex(
    const std::string& key,
    const std::string& filePath,
    const Index& index
) {

    auto it =
        index.find(key);

    if (it == index.end()) {
        return "";
    }

    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open()) {
        return "";
    }

    file.seekg(
        it->second
    );

    std::string line;

    if (!std::getline(file, line)) {
        return "";
    }

    std::size_t separator =
        line.find('|');

    if (
        separator ==
        std::string::npos
    ) {
        return "";
    }

    return line.substr(
        separator + 1
    );
}


std::string SSTable::read(
    const std::string& key,
    const std::string& filePath
) {

    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open()) {
        return "";
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

        std::string storedKey =
            line.substr(
                0,
                separator
            );

        std::string storedValue =
            line.substr(
                separator + 1
            );

        if (storedKey == key) {
            return storedValue;
        }
    }

    return "";
}