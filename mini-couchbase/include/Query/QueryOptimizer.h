
#pragma once

#include "index/IndexClient.h"

#include <cstddef>
#include <string>
#include <vector>

enum class PlanType {
    FullScan,
    IndexScan
};

struct QueryPredicate {
    std::string field;
    std::string value;
};

struct QueryPlan {
    PlanType type = PlanType::FullScan;

    std::string indexName;
    std::string field;
    std::string value;

    double estimatedCost = 0.0;
    std::size_t estimatedRows = 0;
};

class QueryOptimizer {
public:
    QueryPlan optimize(
        const QueryPredicate& predicate,
        const std::vector<IndexMetadata>& indexes,
        std::size_t documentCount,
        double estimatedSelectivity = 0.10
    ) const;
};
