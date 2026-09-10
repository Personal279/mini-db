#include <iostream>

#include "api/HttpServer.h"
#include "client/Client.h"
#include "cluster/ClusterMetadata.h"
#include "cluster/Node.h"
#include "cluster/Partition.h"
#include "node/PartitionManager.h"
#include "node/RequestServer.h"
#include "router/PartitionRouter.h"
#include "router/RoutingService.h"
#include "storage/StorageEngine.h"

int main() {
    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");

    ClusterMetadata metadata;

    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);

    metadata.addPartition(
        Partition(0, {0, 1, 2})
    );

    metadata.addPartition(
        Partition(1, {1, 2, 0})
    );

    metadata.addPartition(
        Partition(2, {2, 0, 1})
    );

    metadata.addPartition(
        Partition(3, {0, 1, 2})
    );

    const int partitionCount = 4;

    PartitionRouter partitionRouter(
        partitionCount
    );

    RoutingService routingService(
        partitionRouter,
        metadata
    );

    StorageEngine storageA(
        "data/nodeA/wal.log",
        "data/nodeA",
        3
    );

    StorageEngine storageB(
        "data/nodeB/wal.log",
        "data/nodeB",
        3
    );

    StorageEngine storageC(
        "data/nodeC/wal.log",
        "data/nodeC",
        3
    );

    PartitionManager partitionManagerA(storageA);
    PartitionManager partitionManagerB(storageB);
    PartitionManager partitionManagerC(storageC);

    partitionManagerA.addPartition(0);
    partitionManagerA.addPartition(3);

    partitionManagerB.addPartition(1);

    partitionManagerC.addPartition(2);

    RequestServer serverA(
        partitionManagerA,
        partitionCount
    );

    RequestServer serverB(
        partitionManagerB,
        partitionCount
    );

    RequestServer serverC(
        partitionManagerC,
        partitionCount
    );

    Client client;

    std::cout << "=============================" << std::endl;
    std::cout << "       MDDB SERVER" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "HTTP API running on:" << std::endl;
    std::cout << "http://localhost:8080" << std::endl;

    HttpServer httpServer(storageA);
    httpServer.start();

    return 0;
}