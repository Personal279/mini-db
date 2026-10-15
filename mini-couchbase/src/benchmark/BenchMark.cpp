
#include <algorithm>
#include <chrono>
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

double percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) {
        return 0.0;
    }

    const auto index = static_cast<std::size_t>(
        p * static_cast<double>(sorted.size() - 1)
    );

    return sorted[index];
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

    double totalUs = 0.0;

    for (double latency : latencies) {
        totalUs += latency;
    }

    result.averageUs = totalUs / latencies.size();
    result.throughput = latencies.size() / elapsedSeconds;

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

template <typename Operation>
BenchmarkResult runBenchmark(
    const std::string& name,
    std::size_t count,
    Operation operation
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
        // This directory must contain benchmark data only.
        const std::filesystem::path root = "benchmark_data";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "data");

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "========================================\n"
                  << "       MDDB STORAGE ENGINE BENCHMARK\n"
                  << "========================================\n"
                  << "Operations per phase: " << OPERATIONS << '\n'
                  << "Scan iterations:      " << SCAN_ITERATIONS << '\n';

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
                const std::string key = "key:" + std::to_string(i);
                const std::string value = "value:" + std::to_string(i);

                storage.put(key, value);
            }
        ));

        // 2. GET hit: read existing keys and validate values.
        printResult(runBenchmark(
            "GET - EXISTING KEYS",
            OPERATIONS,
            [&](std::size_t i) {
                const std::string key = "key:" + std::to_string(i);
                const std::string expected =
                    "value:" + std::to_string(i);

                const std::string actual = storage.get(key);

                if (actual != expected) {
                    throw std::runtime_error(
                        "GET validation failed for " + key
                    );
                }
            }
        ));

        // 3. GET miss: read keys that were never inserted.
        printResult(runBenchmark(
            "GET - MISSING KEYS",
            OPERATIONS,
            [&](std::size_t i) {
                const std::string key =
                    "missing:" + std::to_string(i);

                const std::string actual = storage.get(key);

                if (!actual.empty()) {
                    throw std::runtime_error(
                        "Missing-key GET returned a value for " + key
                    );
                }
            }
        ));

        // 4. UPDATE: overwrite existing keys using put().
        printResult(runBenchmark(
            "UPDATE - OVERWRITE",
            OPERATIONS,
            [&](std::size_t i) {
                const std::string key = "key:" + std::to_string(i);
                const std::string value =
                    "updated:" + std::to_string(i);

                storage.put(key, value);
            }
        ));

        // Verify the updated values outside the timed update phase.
        for (std::size_t i = 0; i < OPERATIONS; ++i) {
            const std::string key = "key:" + std::to_string(i);
            const std::string expected =
                "updated:" + std::to_string(i);

            if (storage.get(key) != expected) {
                throw std::runtime_error(
                    "UPDATE validation failed for " + key
                );
            }
        }

        std::cout << "\nUpdate validation passed.\n";

        // 5. SCAN ALL: measure complete scans of the stored documents.
        printResult(runBenchmark(
            "SCAN ALL",
            SCAN_ITERATIONS,
            [&](std::size_t) {
                const auto documents = storage.scanAll();

                if (documents.size() != OPERATIONS) {
                    throw std::runtime_error(
                        "SCAN ALL returned an unexpected document count"
                    );
                }
            }
        ));

        // 6. DELETE: delete each existing key.
        printResult(runBenchmark(
            "DELETE",
            OPERATIONS,
            [&](std::size_t i) {
                const std::string key = "key:" + std::to_string(i);
                storage.remove(key);
            }
        ));

        // Verify deletes outside the timed phase.
        for (std::size_t i = 0; i < OPERATIONS; ++i) {
            const std::string key = "key:" + std::to_string(i);

            if (!storage.get(key).empty()) {
                throw std::runtime_error(
                    "DELETE validation failed for " + key
                );
            }
        }

        std::cout << "\nDelete validation passed.\n";

        // 7. SCAN ALL after deletes.
        printResult(runBenchmark(
            "SCAN ALL - AFTER DELETE",
            SCAN_ITERATIONS,
            [&](std::size_t) {
                const auto documents = storage.scanAll();

                if (!documents.empty()) {
                    throw std::runtime_error(
                        "SCAN ALL returned documents after deleting all keys"
                    );
                }
            }
        ));

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
