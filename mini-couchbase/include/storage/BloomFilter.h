#pragma once

#include <cstddef>
#include <string>
#include <vector>

class BloomFilter {
public:

    BloomFilter(
        std::size_t size
    );

    void add(
        const std::string& key
    );

    bool possiblyContains(
        const std::string& key
    ) const;

private:

    std::vector<bool> bits;

    std::size_t hash1(
        const std::string& key
    ) const;

    std::size_t hash2(
        const std::string& key
    ) const;
};