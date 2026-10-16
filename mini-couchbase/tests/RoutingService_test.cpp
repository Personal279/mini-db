#include <cassert>
#include <iostream>

#include "cluster/ClusterMetadata.h"
#include "cluster/Node.h"
#include "cluster/Partition.h"
#include "router/PartitionRouter.h"
#include "router/RoutingService.h"

int main() {
    // Arrange: create three nodes.
    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");

    ClusterMetadata metadata;
    metadata.addNode(nodeA);
    metadata.addNode(nodeB);
    metadata.addNode(nodeC);

    // Create four partitions.
    metadata.addPartition(Partition(0, {0, 1, 2}));
    metadata.addPartition(Partition(1, {1, 2, 0}));
    metadata.addPartition(Partition(2, {2, 0, 1}));
    metadata.addPartition(Partition(3, {0, 1, 2}));

    PartitionRouter router(4);
    RoutingService routingService(router, metadata);

    // Test: routing should return the leader ID recorded
    // for whichever partition the key hashes to.
    const std::string key = "user:101";

    const int partitionId = router.findPartition(key);
    const Partition* partition = metadata.getPartition(partitionId);

    assert(partition != nullptr);

    const int expectedLeader = partition->getLeaderNodeId();
    Request request;
    request.operation = Operation::GET;
    request.key = key;

    const int actualLeader = routingService.route(request);

    assert(actualLeader == expectedLeader);

    // Test: every valid partition resolves to a registered node.
    for (int id = 0; id < 4; ++id) {
        const Node* leader = metadata.getLeaderNode(id);
        assert(leader != nullptr);
        assert(leader->getId() == metadata.getPartition(id)->getLeaderNodeId());
    }

    std::cout << "RoutingService tests passed.\n";
    return 0;
}