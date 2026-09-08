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

    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");

    ClusterMetadata metadata;

    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);

    metadata.addPartition(Partition(0, 0));
    metadata.addPartition(Partition(1, 1));
    metadata.addPartition(Partition(2, 2));
    metadata.addPartition(Partition(3, 0));

    const int partitionCount = 4;

    RequestRouter router(metadata, partitionCount);

    StorageEngine storageA("wal_node_A.log", 3);
    StorageEngine storageB("wal_node_B.log", 3);
    StorageEngine storageC("wal_node_C.log", 3);

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

    Request request =
        client.createPutRequest(
            "user:123",
            "Harsha"
        );

    int nodeId = router.route(request);

    std::cout
        << "Router selected Node "
        << nodeId
        << std::endl;

    if (nodeId == 0) {
        serverA.handleRequest(request);
    }
    else if (nodeId == 1) {
        serverB.handleRequest(request);
    }
    else if (nodeId == 2) {
        serverC.handleRequest(request);
    }

    std::cout << "\n--- Testing SSTable Pipeline ---\n";

    storageA.put("test:1", "Harsh");
    storageA.put("test:2", "John");
    storageA.put("test:3", "Alice");

    std::cout
        << "test:1 = "
        << storageA.get("test:1")
        << std::endl;

    std::cout
        << "test:2 = "
        << storageA.get("test:2")
        << std::endl;

    std::cout
        << "test:3 = "
        << storageA.get("test:3")
        << std::endl;

    return 0;
}