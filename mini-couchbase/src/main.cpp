#include <iostream>

#include "api/HttpServer.h"

#include "client/Client.h"

#include "cluster/ClusterMetadata.h"
#include "cluster/Node.h"
#include "cluster/Partition.h"

#include "node/PartitionManager.h"
#include "node/RequestServer.h"

#include "router/RequestRouter.h"

#include "storage/StorageEngine.h"


int main() {

    // =========================
    // CREATE NODES
    // =========================

    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");


    // =========================
    // CLUSTER METADATA
    // =========================

    ClusterMetadata metadata;

    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);


    // =========================
    // PARTITIONS
    // =========================

    metadata.addPartition(Partition(0, 0));
    metadata.addPartition(Partition(1, 1));
    metadata.addPartition(Partition(2, 2));
    metadata.addPartition(Partition(3, 0));


    const int partitionCount = 4;


    // =========================
    // REQUEST ROUTER
    // =========================

    RequestRouter router(
        metadata,
        partitionCount
    );


    // =========================
    // STORAGE ENGINES
    // =========================

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


    // =========================
    // PARTITION MANAGERS
    // =========================

    PartitionManager partitionManagerA(storageA);
    PartitionManager partitionManagerB(storageB);
    PartitionManager partitionManagerC(storageC);


    // Node A owns partitions 0 and 3

    partitionManagerA.addPartition(0);
    partitionManagerA.addPartition(3);


    // Node B owns partition 1

    partitionManagerB.addPartition(1);


    // Node C owns partition 2

    partitionManagerC.addPartition(2);


    // =========================
    // REQUEST SERVERS
    // =========================

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


    // =========================
    // CLIENT
    // =========================

    Client client;


    // =========================
    // START HTTP API SERVER
    // =========================

    std::cout << "=============================" << std::endl;
    std::cout << "       MDDB SERVER" << std::endl;
    std::cout << "=============================" << std::endl;

    std::cout << "HTTP API running on:" << std::endl;
    std::cout << "http://localhost:8080" << std::endl;


    // For Version 1 UI, connect the API
    // directly to storageA.
    //
    // Later, when real networked nodes are added,
    // the API can route requests through the cluster.

    HttpServer httpServer(storageA);

    httpServer.start();


    return 0;
}