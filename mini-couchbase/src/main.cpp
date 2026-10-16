// main.cpp
#include <iostream>
#include <thread>

#include "api/HttpServer.h"
#include "client/Client.h"
#include "cluster/ClusterMetadata.h"
#include "cluster/Node.h"
#include "cluster/Partition.h"
#include "network/NodeNetwork.h"
#include "node/PartitionManager.h"
#include "node/RequestServer.h"
#include "raft/ElectionManager.h"
#include "raft/RaftGroup.h"
#include "raft/RaftNetwork.h"
#include "raft/RaftReplicationManager.h"
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

    metadata.addPartition(Partition(0, {0, 1, 2}));
    metadata.addPartition(Partition(1, {1, 2, 0}));
    metadata.addPartition(Partition(2, {2, 0, 1}));
    metadata.addPartition(Partition(3, {0, 1, 2}));

    const int partitionCount = 4;

    PartitionRouter partitionRouter(partitionCount);
    RoutingService routingService(partitionRouter, metadata);

    StorageEngine storageA("data/nodeA/wal.log", "data/nodeA", 3);
    StorageEngine storageB("data/nodeB/wal.log", "data/nodeB", 3);
    StorageEngine storageC("data/nodeC/wal.log", "data/nodeC", 3);

    RaftGroup raftGroupA(0, {1, 2});
    RaftGroup raftGroupB(1, {0, 2});
    RaftGroup raftGroupC(2, {0, 1});

    PartitionManager partitionManagerA(storageA, raftGroupA);
    PartitionManager partitionManagerB(storageB, raftGroupB);
    PartitionManager partitionManagerC(storageC, raftGroupC);

    partitionManagerA.addPartition(0);
    partitionManagerA.addPartition(1);
    partitionManagerA.addPartition(2);
    partitionManagerA.addPartition(3);

    partitionManagerB.addPartition(0);
    partitionManagerB.addPartition(1);
    partitionManagerB.addPartition(2);
    partitionManagerB.addPartition(3);

    partitionManagerC.addPartition(0);
    partitionManagerC.addPartition(1);
    partitionManagerC.addPartition(2);
    partitionManagerC.addPartition(3);

    NodeNetwork networkA("127.0.0.1", 9001);
    NodeNetwork networkB("127.0.0.1", 9002);
    NodeNetwork networkC("127.0.0.1", 9003);

    RaftNetwork raftNetworkA(networkA, 0, raftGroupA);
    RaftNetwork raftNetworkB(networkB, 1, raftGroupB);
    RaftNetwork raftNetworkC(networkC, 2, raftGroupC);

    ElectionManager electionManagerA(raftGroupA, raftNetworkA);
    ElectionManager electionManagerB(raftGroupB, raftNetworkB);
    ElectionManager electionManagerC(raftGroupC, raftNetworkC);

    RaftReplicationManager replicationManagerA(raftGroupA, raftNetworkA, partitionManagerA);
    RaftReplicationManager replicationManagerB(raftGroupB, raftNetworkB, partitionManagerB);
    RaftReplicationManager replicationManagerC(raftGroupC, raftNetworkC, partitionManagerC);

    RequestServer serverA(partitionManagerA, partitionRouter);
    RequestServer serverB(partitionManagerB, partitionRouter);
    RequestServer serverC(partitionManagerC, partitionRouter);

    Client client;

    std::thread networkThreadA([&networkA]() { networkA.start(); });
    std::thread networkThreadB([&networkB]() { networkB.start(); });
    std::thread networkThreadC([&networkC]() { networkC.start(); });

    electionManagerA.startElectionTimer();
    electionManagerB.startElectionTimer();
    electionManagerC.startElectionTimer();

    replicationManagerA.start();
    replicationManagerB.start();
    replicationManagerC.start();

    std::cout << "=============================" << std::endl;
    std::cout << "       MDDB SERVER" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "HTTP API running on:" << std::endl;
    std::cout << "http://localhost:8080" << std::endl;

    HttpServer httpServer(
        {
            std::ref(serverA),
            std::ref(serverB),
            std::ref(serverC)
        },
        routingService,
        {nodeA, nodeB, nodeC}
    );

    httpServer.start();

    replicationManagerA.stop();
    replicationManagerB.stop();
    replicationManagerC.stop();

    electionManagerA.stopElectionTimer();
    electionManagerB.stopElectionTimer();
    electionManagerC.stopElectionTimer();

    if (networkThreadA.joinable()) networkThreadA.detach();
    if (networkThreadB.joinable()) networkThreadB.detach();
    if (networkThreadC.joinable()) networkThreadC.detach();

    return 0;
}