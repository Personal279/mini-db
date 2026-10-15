
#include "query/QueryExecutor.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

int main() {
    const fs::path dir =
        fs::temp_directory_path() / "mddb_query_executor_test";

    fs::remove_all(dir);
    fs::create_directories(dir / "data");

    try {
        {
            StorageEngine storage(
                (dir / "test.wal").string(),
                (dir / "data").string(),
                100
            );

            IndexClient indexClient;

            // Keep C++ storage and the Go index synchronized.
            storage.put(
                "user:1",
                R"({"name":"Alice","city":"Bengaluru","age":"25"})"
            );
            indexClient.upsertDocument(
                "user:1",
                R"({"name":"Alice","city":"Bengaluru","age":"25"})"
            );

            storage.put(
                "user:2",
                R"({"name":"Bob","city":"Mysuru","age":"30"})"
            );
            indexClient.upsertDocument(
                "user:2",
                R"({"name":"Bob","city":"Mysuru","age":"30"})"
            );

            storage.put(
                "user:3",
                R"({"name":"Charlie","city":"Bengaluru","age":"22"})"
            );
            indexClient.upsertDocument(
                "user:3",
                R"({"name":"Charlie","city":"Bengaluru","age":"22"})"
            );

            QueryExecutor executor(storage, indexClient);

            // Test 1: Index scan.
            const auto indexResult = executor.execute(
                {"city", "Bengaluru"},
                3,
                0.10
            );

            assert(indexResult.plan.type == PlanType::IndexScan);
            assert(indexResult.plan.indexName == "city_idx");
            assert(indexResult.documents.size() == 2);

            std::cout << "Index scan returned "
                      << indexResult.documents.size()
                      << " documents.\n";

            // Test 2: Full scan for a field without an index.
            const auto fullResult = executor.execute(
                {"age", "30"},
                3,
                0.10
            );

            assert(fullResult.plan.type == PlanType::FullScan);
            assert(fullResult.documents.size() == 1);
            assert(fullResult.documents[0].first == "user:2");

            std::cout << "Full scan returned "
                      << fullResult.documents.size()
                      << " document.\n";

            std::cout << "All query executor tests passed.\n";

            // Remove test documents from the Go index.
            indexClient.deleteDocument("user:1");
            indexClient.deleteDocument("user:2");
            indexClient.deleteDocument("user:3");
        }
    } catch (const std::exception& error) {
        std::cerr << "Query executor test failed: "
                  << error.what() << '\n';

        fs::remove_all(dir);
        return 1;
    }

    fs::remove_all(dir);
    return 0;
}
