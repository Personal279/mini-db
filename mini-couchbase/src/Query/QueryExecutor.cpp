
#include "query/QueryExecutor.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>

using nlohmann::json;

namespace {

bool matchesPredicate(
    const std::string& documentJson,
    const QueryPredicate& predicate
) {
    try {
        const json document = json::parse(documentJson);

        if (!document.is_object() ||
            !document.contains(predicate.field)) {
            return false;
        }

        const json& fieldValue = document.at(predicate.field);

        // String fields are compared without JSON quotation marks.
        if (fieldValue.is_string()) {
            return fieldValue.get<std::string>() == predicate.value;
        }

        // For numbers, booleans, and other JSON values, compare
        // their serialized representation.
        return fieldValue.dump() == predicate.value;
    } catch (const json::exception&) {
        // Ignore documents that are not valid JSON objects.
        return false;
    }
}

} // namespace

QueryExecutor::QueryExecutor(
    StorageEngine& storage,
    const IndexClient& indexClient
)
    : storage(storage),
      indexClient(indexClient) {}

QueryResult QueryExecutor::execute(
    const QueryPredicate& predicate,
    std::size_t estimatedDocumentCount,
    double estimatedSelectivity
) const {
    const auto indexes = indexClient.listIndexes();

    QueryPlan plan = optimizer.optimize(
        predicate,
        indexes,
        estimatedDocumentCount,
        estimatedSelectivity
    );

    if (plan.type == PlanType::FullScan) {
        return executeFullScan(predicate, plan);
    }

    try {
        const auto keys = indexClient.lookup(
            plan.indexName,
            predicate.value
        );

        QueryResult result;
        result.plan = plan;

        for (const auto& key : keys) {
            const std::string documentJson = storage.get(key);

            // An empty value is treated as a missing document.
            if (documentJson.empty()) {
                continue;
            }

            // Recheck the predicate against the stored document.
            // This protects against stale or incorrect extra index entries.
            if (matchesPredicate(documentJson, predicate)) {
                result.documents.emplace_back(key, documentJson);
            }
        }

        return result;
    } catch (const std::exception& error) {
        std::cerr
            << "Index query failed; falling back to full scan: "
            << error.what() << '\n';

        QueryPlan fallbackPlan = plan;
        fallbackPlan.type = PlanType::FullScan;
        fallbackPlan.indexName.clear();

        return executeFullScan(predicate, fallbackPlan);
    }
}

QueryResult QueryExecutor::executeFullScan(
    const QueryPredicate& predicate,
    const QueryPlan& plan
) const {
    QueryResult result;
    result.plan = plan;

    for (const auto& [key, documentJson] : storage.scanAll()) {
        if (matchesPredicate(documentJson, predicate)) {
            result.documents.emplace_back(key, documentJson);
        }
    }

    result.plan.estimatedRows = result.documents.size();
    return result;
}
