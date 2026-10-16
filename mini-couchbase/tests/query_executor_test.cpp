
#include "query/QueryExecutor.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;
using Documents = std::vector<std::pair<std::string, std::string>>;

namespace {

constexpr std::size_t DOCUMENT_COUNT = 100;
constexpr std::size_t BENCHMARK_ITERATIONS = 50;

const std::string PREFIX = "query_scan_test:";

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Independent baseline: scan every stored document and apply
// the predicate without using the index or query optimizer.
Documents runFullScan(
    StorageEngine& storage,
    const QueryPredicate& predicate
) {
    Documents matches;

    for (const auto& [key, value] : storage.scanAll()) {
        try {
            const auto document = json::parse(value);

            if (!document.is_object()) {
                continue;
            }

            const auto field = document.find(predicate.field);

            if (field == document.end()) {
                continue;
            }

            // Match the string-valued fields used by this test.
            if (field->is_string() &&
                field->get<std::string>() == predicate.value) {
                matches.emplace_back(key, value);
            }
        } catch (const json::exception&) {
            // Invalid JSON cannot satisfy this test's field predicate.
        }
    }

    std::sort(matches.begin(), matches.end());
    return matches;
}

void requireSameResults(
    Documents actual,
    Documents expected,
    const std::string& description
) {
    std::sort(actual.begin(), actual.end());
    std::sort(expected.begin(), expected.end());

    if (actual == expected) {
        return;
    }

    std::cerr << "\nMismatch: " << description << '\n'
              << "Expected documents: " << expected.size() << '\n'
              << "Actual documents:   " << actual.size() << '\n';

    std::size_t i = 0;
    while (i < actual.size() &&
           i < expected.size() &&
           actual[i] == expected[i]) {
        ++i;
    }

    if (i < expected.size()) {
        std::cerr << "Expected at first mismatch: "
                  << expected[i].first << " -> "
                  << expected[i].second << '\n';
    }

    if (i < actual.size()) {
        std::cerr << "Actual at first mismatch:   "
                  << actual[i].first << " -> "
                  << actual[i].second << '\n';
    }

    throw std::runtime_error(
        description + ": index/full-scan result sets differ"
    );
}

template <typename Function>
double benchmarkMicroseconds(
    Function&& function,
    std::size_t iterations
) {
    // Warm up once before timing.
    function();

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        function();
    }

    const auto end = std::chrono::steady_clock::now();

    const auto total =
        std::chrono::duration<double, std::micro>(end - start).count();

    return total / static_cast<double>(iterations);
}

} // namespace

int main() {
    const fs::path dir =
        fs::temp_directory_path() / "mddb_query_executor_test";

    fs::remove_all(dir);
    fs::create_directories(dir / "data");

    IndexClient indexClient;
    std::vector<std::string> insertedKeys;

    try {
        {
            StorageEngine storage(
                (dir / "test.wal").string(),
                (dir / "data").string(),
                100
            );

            // Seed enough documents to make the comparison meaningful.
            // The indexed field has predictable selectivity.
            for (std::size_t i = 0; i < DOCUMENT_COUNT; ++i) {
                const std::string key =
                    PREFIX + std::to_string(i);

                const std::string city =
                    (i % 4 == 0) ? "Bengaluru"
                    : (i % 4 == 1) ? "Mysuru"
                    : (i % 4 == 2) ? "Delhi"
                                   : "Udupi";

                const std::string document = json{
                    {"name", "User" + std::to_string(i)},
                    {"city", city},
                    {"age", std::to_string(20 + (i % 40))}
                }.dump();

                storage.put(key, document);
                indexClient.upsertDocument(key, document);
                insertedKeys.push_back(key);
            }

            QueryExecutor executor(storage, indexClient);

            const QueryPredicate predicate{"city", "Bengaluru"};

            // 1. Build an independent full-scan baseline.
            const Documents fullScanResults =
                runFullScan(storage, predicate);

            require(
                fullScanResults.size() == DOCUMENT_COUNT / 4,
                "Unexpected full-scan baseline count"
            );

            // 2. Execute the same predicate through the query executor.
            const QueryResult indexResult =
                executor.execute(predicate, DOCUMENT_COUNT, 0.25);

            require(
                indexResult.plan.type == PlanType::IndexScan,
                "Expected IndexScan, but the optimizer selected FullScan"
            );

            require(
                indexResult.plan.indexName == "city_idx",
                "Expected city_idx, got: " + indexResult.plan.indexName
            );

            requireSameResults(
                indexResult.documents,
                fullScanResults,
                "Initial index scan versus full scan"
            );

            std::cout << "Correctness comparison passed.\n"
                      << "Index: " << indexResult.plan.indexName << '\n'
                      << "Matching documents: "
                      << fullScanResults.size() << '\n';

            // 3. Benchmark both paths independently.
            // Each timed iteration executes the same predicate.
            const double indexUs = benchmarkMicroseconds(
                [&]() {
                    const QueryResult result =
                        executor.execute(predicate, DOCUMENT_COUNT, 0.25);

                    require(
                        result.plan.type == PlanType::IndexScan,
                        "Benchmark iteration did not use IndexScan"
                    );

                    requireSameResults(
                        result.documents,
                        fullScanResults,
                        "Timed index scan"
                    );
                },
                BENCHMARK_ITERATIONS
            );

            const double fullScanUs = benchmarkMicroseconds(
                [&]() {
                    const Documents result =
                        runFullScan(storage, predicate);

                    requireSameResults(
                        result,
                        fullScanResults,
                        "Timed full scan"
                    );
                },
                BENCHMARK_ITERATIONS
            );

            std::cout << std::fixed << std::setprecision(2)
                      << "\n========== SCAN COMPARISON ==========\n"
                      << "Documents in dataset: " << DOCUMENT_COUNT << '\n'
                      << "Matching documents:   "
                      << fullScanResults.size() << '\n'
                      << "Iterations per path:  "
                      << BENCHMARK_ITERATIONS << '\n'
                      << "Index scan average:   " << indexUs << " us\n"
                      << "Full scan average:    " << fullScanUs << " us\n";

            if (indexUs > 0.0) {
                std::cout << "Full/index time ratio: "
                          << fullScanUs / indexUs << "x\n";
            }

            if (indexUs < fullScanUs) {
                std::cout << "Index scan was faster in this run.\n";
            } else {
                std::cout << "Full scan was faster in this run.\n";
            }

            std::cout << "All query scan tests passed.\n";
        }

        // Remove only the documents inserted by this test.
        for (const auto& key : insertedKeys) {
            indexClient.deleteDocument(key);
        }

        fs::remove_all(dir);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "\nQuery scan test failed: "
                  << error.what() << '\n';

        // Best-effort cleanup of index entries created by this test.
        for (const auto& key : insertedKeys) {
            try {
                indexClient.deleteDocument(key);
            } catch (...) {
                // Preserve the original test failure.
            }
        }

        fs::remove_all(dir);
        return 1;
    }
}
