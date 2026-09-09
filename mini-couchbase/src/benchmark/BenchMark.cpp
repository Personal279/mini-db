#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "storage/StorageEngine.h"


double calculatePercentile(
    std::vector<double>& latencies,
    double percentile
) {
    if (latencies.empty()) {
        return 0.0;
    }

    std::sort(
        latencies.begin(),
        latencies.end()
    );

    std::size_t index =
        static_cast<std::size_t>(
            percentile *
            (latencies.size() - 1)
        );

    return latencies[index];
}


void printLatencyStats(
    const std::string& operation,
    std::vector<double>& latencies,
    double totalSeconds
) {
    double totalLatency = 0.0;

    for (double latency : latencies) {
        totalLatency += latency;
    }

    double averageLatency =
        totalLatency /
        latencies.size();

    double p50 =
        calculatePercentile(
            latencies,
            0.50
        );

    double p95 =
        calculatePercentile(
            latencies,
            0.95
        );

    double p99 =
        calculatePercentile(
            latencies,
            0.99
        );

    double throughput =
        latencies.size() /
        totalSeconds;

    std::cout << "\n"
              << operation
              << " Results\n";

    std::cout << "Operations: "
              << latencies.size()
              << "\n";

    std::cout << "Time: "
              << totalSeconds
              << " seconds\n";

    std::cout << "Throughput: "
              << throughput
              << " ops/sec\n";

    std::cout << "\nLatency:\n";

    std::cout << "Average: "
              << averageLatency
              << " us\n";

    std::cout << "P50: "
              << p50
              << " us\n";

    std::cout << "P95: "
              << p95
              << " us\n";

    std::cout << "P99: "
              << p99
              << " us\n";
}


int main() {

    const int OPERATIONS = 100000;

    std::cout
        << "=============================\n";

    std::cout
        << "      MDDB BENCHMARK\n";

    std::cout
        << "=============================\n\n";


    StorageEngine storage(
        "benchmark_data/wal.log",
        "benchmark_data",
        1000
    );


    // =====================================
    // PUT BENCHMARK
    // =====================================

    std::cout
        << "Running PUT benchmark...\n";

    std::vector<double> putLatencies;

    putLatencies.reserve(
        OPERATIONS
    );

    auto putStart =
        std::chrono::high_resolution_clock::now();


    for (
        int i = 0;
        i < OPERATIONS;
        i++
    ) {

        auto operationStart =
            std::chrono::high_resolution_clock::now();

        storage.put(
            "key:" + std::to_string(i),
            "value:" + std::to_string(i)
        );

        auto operationEnd =
            std::chrono::high_resolution_clock::now();

        double latency =
            std::chrono::duration<
                double,
                std::micro
            >(
                operationEnd -
                operationStart
            ).count();

        putLatencies.push_back(
            latency
        );
    }


    auto putEnd =
        std::chrono::high_resolution_clock::now();

    double putSeconds =
        std::chrono::duration<double>(
            putEnd - putStart
        ).count();


    printLatencyStats(
        "PUT",
        putLatencies,
        putSeconds
    );


    // =====================================
    // GET BENCHMARK
    // =====================================

    std::cout
        << "\nRunning GET benchmark...\n";

    std::vector<double> getLatencies;

    getLatencies.reserve(
        OPERATIONS
    );

    auto getStart =
        std::chrono::high_resolution_clock::now();


    for (
        int i = 0;
        i < OPERATIONS;
        i++
    ) {

        auto operationStart =
            std::chrono::high_resolution_clock::now();

        std::string result =
            storage.get(
                "key:" +
                std::to_string(i)
            );

        auto operationEnd =
            std::chrono::high_resolution_clock::now();

        double latency =
            std::chrono::duration<
                double,
                std::micro
            >(
                operationEnd -
                operationStart
            ).count();

        getLatencies.push_back(
            latency
        );
    }


    auto getEnd =
        std::chrono::high_resolution_clock::now();

    double getSeconds =
        std::chrono::duration<double>(
            getEnd - getStart
        ).count();


    printLatencyStats(
        "GET",
        getLatencies,
        getSeconds
    );


    // =====================================
    // DELETE BENCHMARK
    // =====================================

    std::cout
        << "\nRunning DELETE benchmark...\n";

    std::vector<double> deleteLatencies;

    deleteLatencies.reserve(
        OPERATIONS
    );

    auto deleteStart =
        std::chrono::high_resolution_clock::now();


    for (
        int i = 0;
        i < OPERATIONS;
        i++
    ) {

        auto operationStart =
            std::chrono::high_resolution_clock::now();

        storage.remove(
            "key:" +
            std::to_string(i)
        );

        auto operationEnd =
            std::chrono::high_resolution_clock::now();

        double latency =
            std::chrono::duration<
                double,
                std::micro
            >(
                operationEnd -
                operationStart
            ).count();

        deleteLatencies.push_back(
            latency
        );
    }


    auto deleteEnd =
        std::chrono::high_resolution_clock::now();

    double deleteSeconds =
        std::chrono::duration<double>(
            deleteEnd - deleteStart
        ).count();


    printLatencyStats(
        "DELETE",
        deleteLatencies,
        deleteSeconds
    );


    std::cout
        << "\n=============================\n";

    std::cout
        << "   BENCHMARK COMPLETE\n";

    std::cout
        << "=============================\n";


    return 0;
}