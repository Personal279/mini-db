
#pragma once

#include "query/QueryOptimizer.h"
#include "storage/StorageEngine.h"
#include "index/IndexClient.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

struct QueryResult {
    QueryPlan plan;
    std::vector<std::pair<std::string, std::string>> documents;
};

class QueryExecutor {
public:
    QueryExecutor(StorageEngine& storage, const IndexClient& indexClient);

    QueryResult execute(
        const QueryPredicate& predicate,
        std::size_t estimatedDocumentCount,
        double estimatedSelectivity = 0.10
    ) const;

private:
    StorageEngine& storage;
    const IndexClient& indexClient;
    QueryOptimizer optimizer;

    QueryResult executeFullScan(
        const QueryPredicate& predicate,
        const QueryPlan& plan
    ) const;
};
