
#include "storage/SSTable.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Remove a trailing carriage return when reading CRLF-formatted lines.
namespace {
void removeTrailingCarriageReturn(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}
}

bool SSTable::write(
    const MemTable& memTable,
    const std::string& filePath
) {
    std::ofstream file(
        filePath,
        std::ios::binary | std::ios::trunc
    );

    if (!file.is_open()) {
        std::cerr << "Failed to open SSTable for writing: "
                  << filePath << '\n';
        return false;
    }

    std::vector<std::pair<std::string, std::string>> entries;

    for (const auto& entry : memTable.getData()) {
        entries.push_back(entry);
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        }
    );

    for (const auto& entry : entries) {
        file << entry.first << '|' << entry.second << '\n';

        if (!file) {
            std::cerr << "Failed while writing SSTable: "
                      << filePath << '\n';
            return false;
        }
    }

    file.flush();

    if (!file) {
        std::cerr << "Failed to flush SSTable: "
                  << filePath << '\n';
        return false;
    }

    return true;
}

SSTable::Index SSTable::buildIndex(
    const std::string& filePath
) {
    Index index;

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open SSTable for indexing: "
                  << filePath << '\n';
        return index;
    }

    while (true) {
        const std::streampos position = file.tellg();

        std::string line;

        if (!std::getline(file, line)) {
            break;
        }

        removeTrailingCarriageReturn(line);

        const std::size_t separator = line.find('|');

        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, separator);
        index[key] = position;
    }

    return index;
}

BloomFilter SSTable::buildBloomFilter(
    const std::string& filePath,
    std::size_t filterSize
) {
    BloomFilter bloomFilter(filterSize);

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open SSTable for Bloom filter: "
                  << filePath << '\n';
        return bloomFilter;
    }

    std::string line;

    while (std::getline(file, line)) {
        removeTrailingCarriageReturn(line);

        const std::size_t separator = line.find('|');

        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, separator);
        bloomFilter.add(key);
    }

    return bloomFilter;
}

std::string SSTable::readWithIndex(
    const std::string& key,
    const std::string& filePath,
    const Index& index
) {
    const auto it = index.find(key);

    if (it == index.end()) {
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open SSTable for indexed read: "
                  << filePath << '\n';
        return "";
    }

    file.seekg(it->second);

    if (!file) {
        std::cerr << "Failed to seek to indexed SSTable entry: "
                  << filePath << '\n';
        return "";
    }

    std::string line;

    if (!std::getline(file, line)) {
        return "";
    }

    removeTrailingCarriageReturn(line);

    const std::size_t separator = line.find('|');

    if (separator == std::string::npos) {
        return "";
    }

    return line.substr(separator + 1);
}

std::vector<std::pair<std::string, std::string>> SSTable::readAll(
    const std::string& filePath
) {
    std::vector<std::pair<std::string, std::string>> entries;

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Cannot open SSTable: "
                  << filePath << '\n';
        return entries;
    }

    std::string line;

    while (std::getline(file, line)) {
        removeTrailingCarriageReturn(line);

        const std::size_t separator = line.find('|');

        if (separator == std::string::npos) {
            std::cerr << "Invalid SSTable line: missing '|'\n";
            continue;
        }

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator + 1);

        entries.emplace_back(std::move(key), std::move(value));
    }

    return entries;
}

std::string SSTable::read(
    const std::string& key,
    const std::string& filePath
) {
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        return "";
    }

    std::string line;

    while (std::getline(file, line)) {
        removeTrailingCarriageReturn(line);

        const std::size_t separator = line.find('|');

        if (separator == std::string::npos) {
            continue;
        }

        const std::string storedKey = line.substr(0, separator);
        const std::string storedValue = line.substr(separator + 1);

        if (storedKey == key) {
            return storedValue;
        }
    }

    return "";
}
