
#include "storage/StorageEngine.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using Entries = std::map<std::string, std::string>;

const Entries expected = {
    {"user:1", "newest"},
    {"user:3", "three"},
    {"user:4", "four"},
    {"user:5", "five"},
    {"user:6", "six"},
    {"user:7", "seven"},
    {"user:8", "eight"}
};

Entries collectEntries(StorageEngine& engine) {
    const auto entries = engine.scanAll();

    return Entries(entries.begin(), entries.end());
}

bool verifyEntries(
    const Entries& actual,
    const Entries& expectedEntries,
    const std::string& testName
) {
    if (actual == expectedEntries) {
        std::cout << "[PASS] " << testName << '\n';
        return true;
    }

    std::cerr << "[FAIL] " << testName << '\n';
    std::cerr << "Expected count: " << expectedEntries.size() << '\n';
    std::cerr << "Actual count:   " << actual.size() << '\n';

    std::cerr << "\nExpected entries:\n";
    for (const auto& [key, value] : expectedEntries) {
        std::cerr << '[' << key << "] -> [" << value << "]\n";
    }

    std::cerr << "\nActual entries:\n";
    for (const auto& [key, value] : actual) {
        std::cerr << '[' << key << "] -> [" << value << "]\n";
    }

    return false;
}

int main() {
    const fs::path testDir =
        fs::temp_directory_path() / "mddb_compaction_recovery_test";

    const fs::path walPath = testDir / "test.wal";
    const fs::path dataDir = testDir / "data";

    // Begin with a clean test directory.
    fs::remove_all(testDir);
    fs::create_directories(dataDir);

    bool initialTestPassed = false;
    bool recoveryTestPassed = false;

    // Phase 1: write, overwrite, delete, flush and compact.
    {
        StorageEngine engine(
            walPath.string(),
            dataDir.string(),
            2
        );

        engine.put("user:1", "old");
        engine.put("user:2", "keep");

        engine.put("user:3", "three");
        engine.put("user:4", "four");

        engine.put("user:1", "newest");
        engine.remove("user:2");

        engine.put("user:5", "five");
        engine.put("user:6", "six");
        engine.put("user:7", "seven");
        engine.put("user:8", "eight");

        // Allow asynchronous flush and compaction to progress.
        std::this_thread::sleep_for(std::chrono::seconds(2));

        const Entries actual = collectEntries(engine);

        initialTestPassed = verifyEntries(
            actual,
            expected,
            "Values and tombstones before restart"
        );
    }

    // Phase 2: the first engine is destroyed.
    // Reopen the same WAL and SSTable directory.
    {
        StorageEngine recoveredEngine(
            walPath.string(),
            dataDir.string(),
            2
        );

        const Entries recovered = collectEntries(recoveredEngine);

        recoveryTestPassed = verifyEntries(
            recovered,
            expected,
            "Values and tombstones after restart"
        );

        // Explicitly check the overwritten and deleted keys.
        const std::string latestValue = recoveredEngine.get("user:1");
        const std::string deletedValue = recoveredEngine.get("user:2");

        if (latestValue != "newest") {
            std::cerr << "[FAIL] Overwritten value after restart: "
                      << "expected [newest], got ["
                      << latestValue << "]\n";
            recoveryTestPassed = false;
        } else {
            std::cout << "[PASS] Latest overwritten value survives restart\n";
        }

        if (!deletedValue.empty()) {
            std::cerr << "[FAIL] Deleted key returned value ["
                      << deletedValue << "] after restart\n";
            recoveryTestPassed = false;
        } else {
            std::cout << "[PASS] Deleted key remains absent after restart\n";
        }
    }

    // Report the overall result.
    if (initialTestPassed && recoveryTestPassed) {
        std::cout << "\nAll compaction and recovery tests passed.\n";
        fs::remove_all(testDir);
        return 0;
    }

    std::cerr << "\nOne or more tests failed. "
              << "Keeping test files for investigation at:\n"
              << testDir.string() << '\n';

    return 1;
}
