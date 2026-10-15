
#include "query/QueryOptimizer.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    QueryOptimizer optimizer;

    std::vector<IndexMetadata> indexes = {
        {"city_idx", "city"}
    };

    // Case 1: A selective predicate should use the index.
    auto indexPlan = optimizer.optimize(
        {"city", "Bengaluru"},
        indexes,
        1000,
        0.10
    );

    assert(indexPlan.type == PlanType::IndexScan);
    assert(indexPlan.indexName == "city_idx");

    std::cout << "Selective predicate: IndexScan\n";

    // Case 2: Without a matching index, use a full scan.
    auto fullPlan = optimizer.optimize(
        {"age", "25"},
        indexes,
        1000,
        0.10
    );

    assert(fullPlan.type == PlanType::FullScan);

    std::cout << "No matching index: FullScan\n";

    // Case 3: A non-selective predicate may favor a full scan.
    auto broadPlan = optimizer.optimize(
        {"city", "Bengaluru"},
        indexes,
        1000,
        1.0
    );

    assert(broadPlan.type == PlanType::FullScan);

    std::cout << "Broad predicate: FullScan\n";

    std::cout << "All optimizer tests passed.\n";
    return 0;
}
