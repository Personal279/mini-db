
#include "storage/StorageEngine.h"
#include <stdexcept>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

void writeSSTable(
    const fs::path& path,
    const std::vector<std::pair<std::string, std::string>>& entries
) {
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to create SSTable: " + path.string()
        );
    }

    for (const auto& [key, value] : entries) {
        file << key << "|" << value << "\n";
    }
}

int main() {
    const fs::path dir =
        fs::temp_directory_path() / "mddb_scan_test";

    // Remove data left by any previous test run.
    fs::remove_all(dir);
    fs::create_directories(dir);

    const fs::path walPath = dir / "test.wal";
    const fs::path dataDir = dir / "data";

    fs::create_directories(dataDir);

    // Oldest SSTable.
    writeSSTable(dataDir / "sstable_0.db", {
        {"user:1", "old"},
        {"user:2", "older"},
        {"user:3", "keep"}
    });

    // Newer SSTable overwrites user:1 and deletes user:2.
    writeSSTable(dataDir / "sstable_1.db", {
        {"user:1", "new"},
        {"user:2", MemTable::TOMBSTONE},
        {"user:4", "from_sstable"}
    });

    {
        StorageEngine engine(
            walPath.string(),
            dataDir.string(),
            100
        );

        // Active MemTable has the newest version of user:1.
        engine.put("user:1", "newest");
        engine.put("user:5", "memtable_only");

        const auto entries = engine.scanAll();

        const std::map<std::string, std::string> actual(
            entries.begin(),
            entries.end()
        );

        // Print the actual result before asserting.
        std::cout << "\nActual entries (" << actual.size()
                  << "):\n";

        for (const auto& [key, value] : actual) {
            std::cout << key << " -> " << value << "\n";
        }

        // Expected visible state.
        const std::map<std::string, std::string> expected = {
            {"user:1", "newest"},
            {"user:3", "keep"},
            {"user:4", "from_sstable"},
            {"user:5", "memtable_only"}
        };

        if (actual != expected) {
            std::cerr << "\nSCAN TEST FAILED\n";
            return 1;
        }

        std::cout << "\nAll scan merge tests passed.\n";
    }

    fs::remove_all(dir);
    return 0;
}
