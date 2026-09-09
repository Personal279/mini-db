#include <chrono>
#include <iostream>
#include <string>

#include "storage/StorageEngine.h"


int main() {

    const int OPERATIONS = 100000;

    std::cout << "=============================\n";
    std::cout << "      MDDB BENCHMARK\n";
    std::cout << "=============================\n\n";


    StorageEngine storage(
        "benchmark_data/wal.log",
        "benchmark_data",
        1000
    );


    // =========================
    // PUT BENCHMARK
    // =========================

    std::cout << "Running PUT benchmark...\n";

    auto putStart =
        std::chrono::high_resolution_clock::now();


    for (int i = 0; i < OPERATIONS; i++) {

        storage.put(
            "key:" + std::to_string(i),
            "value:" + std::to_string(i)
        );
    }


    auto putEnd =
        std::chrono::high_resolution_clock::now();


    std::chrono::duration<double> putDuration =
        putEnd - putStart;


    double putOpsPerSecond =
        OPERATIONS / putDuration.count();


    std::cout << "\nPUT Results\n";

    std::cout
        << "Operations: "
        << OPERATIONS
        << "\n";

    std::cout
        << "Time: "
        << putDuration.count()
        << " seconds\n";

    std::cout
        << "Throughput: "
        << putOpsPerSecond
        << " ops/sec\n";

    std::cout << "\nRunning GET benchmark...\n";


    auto getStart =
        std::chrono::high_resolution_clock::now();


    for (int i = 0; i < OPERATIONS; i++) {

        storage.get(
            "key:" + std::to_string(i)
        );
    }


    auto getEnd =
        std::chrono::high_resolution_clock::now();


    std::chrono::duration<double> getDuration =
        getEnd - getStart;


    double getOpsPerSecond =
        OPERATIONS / getDuration.count();


    std::cout << "\nGET Results\n";

    std::cout
        << "Operations: "
        << OPERATIONS
        << "\n";

    std::cout
        << "Time: "
        << getDuration.count()
        << " seconds\n";

    std::cout
        << "Throughput: "
        << getOpsPerSecond
        << " ops/sec\n";


    // =========================
    // DELETE BENCHMARK
    // =========================

    std::cout << "\nRunning DELETE benchmark...\n";


    auto deleteStart =
        std::chrono::high_resolution_clock::now();


    for (int i = 0; i < OPERATIONS; i++) {

        storage.remove(
            "key:" + std::to_string(i)
        );
    }


    auto deleteEnd =
        std::chrono::high_resolution_clock::now();


    std::chrono::duration<double> deleteDuration =
        deleteEnd - deleteStart;


    double deleteOpsPerSecond =
        OPERATIONS / deleteDuration.count();


    std::cout << "\nDELETE Results\n";

    std::cout
        << "Operations: "
        << OPERATIONS
        << "\n";

    std::cout
        << "Time: "
        << deleteDuration.count()
        << " seconds\n";

    std::cout
        << "Throughput: "
        << deleteOpsPerSecond
        << " ops/sec\n";


    std::cout << "\n=============================\n";
    std::cout << "   BENCHMARK COMPLETE\n";
    std::cout << "=============================\n";


    return 0;
}