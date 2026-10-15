
#include "query/QueryOptimizer.h"

#include <algorithm>
#include <cmath>

QueryPlan QueryOptimizer::optimize(
    const QueryPredicate& predicate,
    const std::vector<IndexMetadata>& indexes,
    std::size_t documentCount,
    double estimatedSelectivity
) const {
    QueryPlan fullScan;
    fullScan.type = PlanType::FullScan;
    fullScan.field = predicate.field;
    fullScan.value = predicate.value;
    fullScan.estimatedRows = documentCount;
    fullScan.estimatedCost = static_cast<double>(documentCount);

    // No documents means a full scan is already cheap.
    if (documentCount == 0) {
        return fullScan;
    }

    // Clamp the heuristic to a valid selectivity range.
    estimatedSelectivity =
        std::clamp(estimatedSelectivity, 0.0, 1.0);

    const IndexMetadata* matchingIndex = nullptr;

    for (const auto& index : indexes) {
        if (index.field == predicate.field) {
            matchingIndex = &index;
            break;
        }
    }

    // No index supports this predicate.
    if (matchingIndex == nullptr) {
        return fullScan;
    }

    QueryPlan indexScan;
    indexScan.type = PlanType::IndexScan;
    indexScan.indexName = matchingIndex->name;
    indexScan.field = predicate.field;
    indexScan.value = predicate.value;

    indexScan.estimatedRows = static_cast<std::size_t>(
        std::ceil(documentCount * estimatedSelectivity)
    );

    // Initial heuristic:
    // index lookup overhead + estimated matching-document fetches.
    constexpr double indexLookupCost = 1.0;
    constexpr double documentFetchCost = 1.0;

    indexScan.estimatedCost =
        indexLookupCost +
        indexScan.estimatedRows * documentFetchCost;

    // Choose the index only when its estimated cost is lower.
    if (indexScan.estimatedCost < fullScan.estimatedCost) {
        return indexScan;
    }

    return fullScan;
}
