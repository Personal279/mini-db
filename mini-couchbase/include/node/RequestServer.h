#pragma once

#include "common/Types.h"
#include "node/PartitionManager.h"
#include "router/PartitionRouter.h"

class RequestServer {
public:
    RequestServer(
        PartitionManager& partitionManager,
        PartitionRouter& partitionRouter
    );

    Response handleRequest(const Request& request);
    QueryResult executeQuery(const QueryPredicate& predicate);

private:
    PartitionManager& partitionManager;
    PartitionRouter& partitionRouter;
};