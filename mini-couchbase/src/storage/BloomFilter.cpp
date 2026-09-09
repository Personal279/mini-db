#include "storage/BloomFilter.h"

#include <functional>


BloomFilter::BloomFilter(
    std::size_t size
)
    : bits(size, false) {
}


std::size_t BloomFilter::hash1(
    const std::string& key
) const {

    return std::hash<std::string>{}(key)
        % bits.size();
}


std::size_t BloomFilter::hash2(
    const std::string& key
) const {

    std::size_t hash = 5381;

    for (char c : key) {

        hash =
            ((hash << 5) + hash)
            + static_cast<unsigned char>(c);
    }

    return hash % bits.size();
}


void BloomFilter::add(
    const std::string& key
) {

    bits[hash1(key)] = true;

    bits[hash2(key)] = true;
}


bool BloomFilter::possiblyContains(
    const std::string& key
) const {

    return
        bits[hash1(key)] &&
        bits[hash2(key)];
}