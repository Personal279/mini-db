#include "storage/SSTable.h"

#include <fstream>
#include <vector>
#include <algorithm>


bool SSTable::write(
    const MemTable& memTable,
    const std::string& filePath
) {
    std::ofstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    std::vector<
        std::pair<std::string, std::string>
    > entries;

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

        file << entry.first
             << "|"
             << entry.second
             << "\n";
    }

    return true;
}