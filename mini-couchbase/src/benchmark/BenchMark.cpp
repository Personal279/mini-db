#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "storage/StorageEngine.h"

using Clock = std::chrono::steady_clock;
using Microseconds = std::chrono::duration<double, std::micro>;

constexpr std::size_t OPERATIONS = 100000;
constexpr std::size_t SCAN_ITERATIONS = 20;

struct BenchmarkResult {
    std::string name;
    std::size_t operations = 0;
    double elapsedSeconds = 0.0;
    double averageUs = 0.0;
    double p50Us = 0.0;
    double p95Us = 0.0;
    double p99Us = 0.0;
    double throughput = 0.0;
};

// Nearest-rank percentile on an already sorted vector.
double percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) {
        return 0.0;
    }

    std::size_t rank = static_cast<std::size_t>(
        std::ceil(p * static_cast<double>(sorted.size()))
    );

    if (rank < 1) {
        rank = 1;
    }

    if (rank > sorted.size()) {
        rank = sorted.size();
    }

    return sorted[rank - 1];
}

BenchmarkResult summarize(
    const std::string& name,
    std::vector<double> latencies,
    double elapsedSeconds
) {
    BenchmarkResult result;
    result.name = name;
    result.operations = latencies.size();
    result.elapsedSeconds = elapsedSeconds;

    if (latencies.empty() || elapsedSeconds <= 0.0) {
        return result;
    }

    const double count = static_cast<double>(latencies.size());

    double totalUs = 0.0;

    for (double latency : latencies) {
        totalUs += latency;
    }

    result.averageUs = totalUs / count;
    result.throughput = count / elapsedSeconds;

    std::sort(latencies.begin(), latencies.end());

    result.p50Us = percentile(latencies, 0.50);
    result.p95Us = percentile(latencies, 0.95);
    result.p99Us = percentile(latencies, 0.99);

    return result;
}

void printResult(const BenchmarkResult& result) {
    std::cout << "\n========== " << result.name << " ==========\n"
              << "Operations:       " << result.operations << '\n'
              << "Elapsed:          " << result.elapsedSeconds << " s\n"
              << "Throughput:       " << result.throughput
              << " ops/sec\n"
              << "Average latency:  " << result.averageUs << " us\n"
              << "P50 latency:      " << result.p50Us << " us\n"
              << "P95 latency:      " << result.p95Us << " us\n"
              << "P99 latency:      " << result.p99Us << " us\n";
}

// Times only operation(i). Callers must prepare all inputs beforehand and
// validate all outputs afterwards, so string building and checking are not
// counted as storage latency.
template <typename Func>
BenchmarkResult runBenchmark(
    const std::string& name,
    std::size_t count,
    Func operation
) {
    std::vector<double> latencies;
    latencies.reserve(count);

    const auto overallStart = Clock::now();

    for (std::size_t i = 0; i < count; ++i) {
        const auto start = Clock::now();

        operation(i);

        const auto end = Clock::now();

        latencies.push_back(
            Microseconds(end - start).count()
        );
    }

    const auto overallEnd = Clock::now();

    const double elapsedSeconds =
        std::chrono::duration<double>(
            overallEnd - overallStart
        ).count();

    return summarize(name, std::move(latencies), elapsedSeconds);
}

int main() {
    try {
        // This directory contains benchmark data only. It is wiped on
        // every run, so the absolute path is printed below.
        const std::filesystem::path root = "benchmark_data";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "data");

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "========================================\n"
                  << "       MDDB STORAGE ENGINE BENCHMARK\n"
                  << "========================================\n"
                  << "Operations per phase: " << OPERATIONS << '\n'
                  << "Scan iterations:      " << SCAN_ITERATIONS << '\n'
                  << "Data directory:       "
                  << std::filesystem::absolute(root).string() << '\n';

        // Build every key and value up front (outside all timed regions).
        std::vector<std::string> keys;
        std::vector<std::string> missingKeys;
        std::vector<std::string> insertValues;
        std::vector<std::string> updatedValues;

        keys.reserve(OPERATIONS);
        missingKeys.reserve(OPERATIONS);
        insertValues.reserve(OPERATIONS);
        updatedValues.reserve(OPERATIONS);

        for (std::size_t i = 0; i < OPERATIONS; ++i) {
            const std::string id = std::to_string(i);

            keys.push_back("key:" + id);
            missingKeys.push_back("missing:" + id);
            insertValues.push_back("value:" + id);
            updatedValues.push_back("updated:" + id);
        }

        // Third argument: same constructor parameter that main.cpp passes
        // as 3. Keep it fixed so runs are comparable.
        StorageEngine storage(
            (root / "wal.log").string(),
            (root / "data").string(),
            1000
        );

        // 1. PUT: insert new keys.
        printResult(runBenchmark(
            "PUT - INSERT",
            OPERATIONS,
            [&](std::size_t i) {
                storage.put(keys[i], insertValues[i]);
            }
        ));

        // 2. GET: read existing keys. Results are validated afterwards.
        {
            std::vector<std::string> results(OPERATIONS);

            printResult(runBenchmark(
                "GET - EXISTING KEYS",
                OPERATIONS,
                [&](std::size_t i) {
                    results[i] = storage.get(keys[i]);
                }
            ));

            for (std::size_t i = 0; i < OPERATIONS; ++i) {
                if (results[i] != insertValues[i]) {
                    throw std::runtime_error(
                        "GET validation failed for " + keys[i] +
                        ": expected [" + insertValues[i] +
                        "], got [" + results[i] + "]"
                    );
                }
            }
        }

        // 3. GET miss: read keys that were never inserted.
        {
            std::vector<std::string> results(OPERATIONS);

            printResult(runBenchmark(
                "GET - MISSING KEYS",
                OPERATIONS,
                [&](std::size_t i) {
                    results[i] = storage.get(missingKeys[i]);
                }
            ));

            for (std::size_t i = 0; i < OPERATIONS; ++i) {
                if (!results[i].empty()) {
                    throw std::runtime_error(
                        "Missing-key GET returned a value for " +
                        missingKeys[i]
                    );
                }
            }
        }

        // 4. UPDATE: overwrite existing keys.
        printResult(runBenchmark(
            "UPDATE - OVERWRITE",
            OPERATIONS,
            [&](std::size_t i) {
                storage.put(keys[i], updatedValues[i]);
            }
        ));

        // Validate updated values outside the timed update phase.
        for (std::size_t i = 0; i < OPERATIONS; ++i) {
            if (storage.get(keys[i]) != updatedValues[i]) {
                throw std::runtime_error(
                    "UPDATE validation failed for " + keys[i]
                );
            }
        }

        std::cout << "\nUpdate validation passed.\n";

        // 5. SCAN ALL: measure complete scans. Document counts are checked
        //    after the timed phase.
        {
            std::vector<std::size_t> counts(SCAN_ITERATIONS, 0);

            printResult(runBenchmark(
                "SCAN ALL",
                SCAN_ITERATIONS,
                [&](std::size_t iteration) {
                    const auto documents = storage.scanAll();
                    counts[iteration] = documents.size();
                }
            ));

            for (std::size_t count : counts) {
                if (count != OPERATIONS) {
                    throw std::runtime_error(
                        "SCAN ALL returned an unexpected document count: " +
                        std::to_string(count)
                    );
                }
            }
        }

        // 6. DELETE: delete each existing key.
        printResult(runBenchmark(
            "DELETE",
            OPERATIONS,
            [&](std::size_t i) {
                storage.remove(keys[i]);
            }
        ));

        // Verify deletes outside the timed phase.
        for (std::size_t i = 0; i < OPERATIONS; ++i) {
            if (!storage.get(keys[i]).empty()) {
                throw std::runtime_error(
                    "DELETE validation failed for " + keys[i]
                );
            }
        }

        std::cout << "\nDelete validation passed.\n";

        // 7. SCAN ALL after deletes.
        {
            std::vector<std::size_t> counts(SCAN_ITERATIONS, 0);

            printResult(runBenchmark(
                "SCAN ALL - AFTER DELETE",
                SCAN_ITERATIONS,
                [&](std::size_t iteration) {
                    const auto documents = storage.scanAll();
                    counts[iteration] = documents.size();
                }
            ));

            for (std::size_t count : counts) {
                if (count != 0) {
                    throw std::runtime_error(
                        "SCAN ALL returned documents after deleting all "
                        "keys: " + std::to_string(count)
                    );
                }
            }
        }

        std::cout << "\n========================================\n"
                  << "       ALL BENCHMARKS COMPLETED\n"
                  << "       All validation checks passed\n"
                  << "========================================\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "\nBENCHMARK FAILED: "
                  << error.what() << '\n';
        return 1;
    }
}