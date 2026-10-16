#include "node/RequestServer.h"

#include <iostream>

RequestServer::RequestServer(
    PartitionManager& partitionManager,
    PartitionRouter& partitionRouter
)
    : partitionManager(partitionManager),
      partitionRouter(partitionRouter) {
}

Response RequestServer::handleRequest(const Request& request) {
    int partitionId = partitionRouter.findPartition(request.key);

    if (!partitionManager.ownsPartition(partitionId)) {
        std::cout << "This node does not own partition "
                  << partitionId << std::endl;

        return {false, false, "", "Does not own partition"};
    }

    std::cout << "Request accepted for partition "
              << partitionId << std::endl;

    return partitionManager.processRequest(request);
}

QueryResult RequestServer::executeQuery(
    const QueryPredicate& predicate
) {
    return partitionManager.executeQuery(predicate);
}