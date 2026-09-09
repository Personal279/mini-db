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

    RequestRouter router(
        metadata,
        partitionCount
    );

    StorageEngine storageA(
        "wal_node_A.log",
        3
    );

    StorageEngine storageB(
        "wal_node_B.log",
        3
    );

    StorageEngine storageC(
        "wal_node_C.log",
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

    std::cout
        << "\n--- Testing SSTable Startup Recovery ---\n";

    {
        StorageEngine testStorage(
            "wal_recovery_test.log",
            2
        );

        testStorage.put(
            "restart:1",
            "Harsh"
        );

        testStorage.put(
            "restart:2",
            "John"
        );

        std::cout
            << "Before restart: "
            << testStorage.get("restart:1")
            << std::endl;
    }

    std::cout
        << "\n--- Simulating Restart ---\n";

    {
        StorageEngine recoveredStorage(
            "wal_recovery_test.log",
            2
        );

        std::string value =
            recoveredStorage.get(
                "restart:1"
            );

        if (value.empty()) {

            std::cout
                << "After restart: NOT FOUND"
                << std::endl;
        }
        else {

            std::cout
                << "After restart: "
                << value
                << std::endl;
        }
    }

    return 0;
}