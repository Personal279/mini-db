#include <iostream>

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
    // 1. Create cluster nodes
    // =========================

    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");


    // =========================
    // 2. Create cluster metadata
    // =========================

    ClusterMetadata metadata;

    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);


    // =========================
    // 3. Assign partitions
    // =========================

    metadata.addPartition(Partition(0, 0));
    metadata.addPartition(Partition(1, 1));
    metadata.addPartition(Partition(2, 2));
    metadata.addPartition(Partition(3, 0));


    // =========================
    // 4. Create router
    // =========================

    const int partitionCount = 4;

    RequestRouter router(metadata, partitionCount);


    // =========================
    // 5. Create storage engines
    // =========================

    StorageEngine storageA;
    StorageEngine storageB;
    StorageEngine storageC;


    // =========================
    // 6. Create PartitionManagers
    // =========================

    PartitionManager partitionManagerA(storageA);
    PartitionManager partitionManagerB(storageB);
    PartitionManager partitionManagerC(storageC);


    // Tell each node which partitions it owns

    partitionManagerA.addPartition(0);
    partitionManagerA.addPartition(3);

    partitionManagerB.addPartition(1);

    partitionManagerC.addPartition(2);


    // =========================
    // 7. Create RequestServers
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
    // 8. Create Client
    // =========================

    Client client;


    // =========================
    // 9. Create PUT request
    // =========================

    Request request =
        client.createPutRequest(
            "user:123",
            "Harsha"
        );


    // =========================
    // 10. Router finds node
    // =========================

    int nodeId = router.route(request);

    std::cout
        << "Router selected Node "
        << nodeId
        << std::endl;


    // =========================
    // 11. Send request to node
    // =========================

    if (nodeId == 0) {
        serverA.handleRequest(request);
    }
    else if (nodeId == 1) {
        serverB.handleRequest(request);
    }
    else if (nodeId == 2) {
        serverC.handleRequest(request);
    }


    return 0;
}