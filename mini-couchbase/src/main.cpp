#include <iostream>

#include "cluster/Node.h"
#include "cluster/Partition.h"
#include "cluster/ClusterMetadata.h"
#include "router/RequestRouter.h"

int main() {
    // 1. Create nodes
    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");

    // 2. Create cluster metadata
    ClusterMetadata metadata;

    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);

    // 3. Create partitions
    // For now: 4 partitions
    metadata.addPartition(Partition(0, 0)); // P0 → Node A
    metadata.addPartition(Partition(1, 1)); // P1 → Node B
    metadata.addPartition(Partition(2, 2)); // P2 → Node C
    metadata.addPartition(Partition(3, 0)); // P3 → Node A

    // 4. Create router
    RequestRouter router(metadata, 4);

    // 5. Test keys
    std::string keys[] = {
        "user:123",
        "user:456",
        "user:789",
        "product:100"
    };

    for (const auto& key : keys) {
        int partition = router.findPartition(key);
        int nodeId = router.findNode(key);

        const Node* node = metadata.getNode(nodeId);

        std::cout
            << "Key: " << key
            << " | Partition: " << partition
            << " | Node: " << node->getId()
            << " | Address: " << node->getAddress()
            << '\n';
    }

    return 0;
}