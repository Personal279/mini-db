#pragma once

#include <cstddef>
#include <ios>
#include <string>
#include <unordered_map>

#include "storage/BloomFilter.h"
#include "storage/MemTable.h"


class SSTable {
public:

    using Index =
        std::unordered_map<
            std::string,
            std::streampos
        >;


    static bool write(
        const MemTable& memTable,
        const std::string& filePath
    );


    static std::string read(
        const std::string& key,
        const std::string& filePath
    );


    static Index buildIndex(
        const std::string& filePath
    );


    static BloomFilter buildBloomFilter(
        const std::string& filePath,
        std::size_t filterSize = 10000
    );


    static std::string readWithIndex(
        const std::string& key,
        const std::string& filePath,
        const Index& index
    );
};