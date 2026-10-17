#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

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
    constexpr int nodeCount = 3;
    constexpr int partitionCount = 4;

    Node nodeA(0, "127.0.0.1:9001");
    Node nodeB(1, "127.0.0.1:9002");
    Node nodeC(2, "127.0.0.1:9003");

    std::array<Node, nodeCount> nodes{nodeA, nodeB, nodeC};

    ClusterMetadata metadata;

    for (const auto& node : nodes) {
        metadata.addNode(node);
    }

    metadata.addPartition(Partition(0, {0, 1}));
    metadata.addPartition(Partition(1, {1, 2}));
    metadata.addPartition(Partition(2, {2, 0}));
    metadata.addPartition(Partition(3, {0, 1}));

    PartitionRouter partitionRouter(partitionCount);
    RoutingService routingService(partitionRouter, metadata);

    StorageEngine storageA("data/nodeA/wal.log", "data/nodeA", 3);
    StorageEngine storageB("data/nodeB/wal.log", "data/nodeB", 3);
    StorageEngine storageC("data/nodeC/wal.log", "data/nodeC", 3);

    std::array<StorageEngine*, nodeCount> storageEngines{
        &storageA,
        &storageB,
        &storageC
    };

    // One possible slot for each node-partition combination.
    // Non-replica combinations remain nullptr.
    constexpr int groupCount = nodeCount * partitionCount;

    auto groupIndex = [partitionCount](int nodeId, int partitionId) {
        return nodeId * partitionCount + partitionId;
    };

    std::array<std::unique_ptr<RaftGroup>, groupCount> groups{};

    // Create Raft groups only for nodes that host each partition.
    for (int partitionId = 0;
         partitionId < partitionCount;
         ++partitionId) {

        const Partition* partition =
            metadata.getPartition(partitionId);

        if (partition == nullptr) {
            throw std::runtime_error("Partition not found");
        }

        const auto& replicaNodeIds =
            partition->getReplicaNodeIds();

        for (int nodeId : replicaNodeIds) {
            if (nodeId < 0 || nodeId >= nodeCount) {
                throw std::runtime_error(
                    "Partition contains an invalid replica node ID"
                );
            }

            std::vector<int> peers;

            for (int replicaNodeId : replicaNodeIds) {
                if (replicaNodeId != nodeId) {
                    peers.push_back(replicaNodeId);
                }
            }

            groups[groupIndex(nodeId, partitionId)] =
                std::make_unique<RaftGroup>(
                    nodeId,
                    partitionId,
                    peers
                );
        }
    }

    // One PartitionManager per node, registering only its local groups.
    std::array<std::unique_ptr<PartitionManager>, nodeCount>
        partitionManagers{};

    for (int nodeId = 0; nodeId < nodeCount; ++nodeId) {
        partitionManagers[nodeId] =
            std::make_unique<PartitionManager>(
                *storageEngines[nodeId],
                partitionRouter
            );

        for (int partitionId = 0;
             partitionId < partitionCount;
             ++partitionId) {

            const int index = groupIndex(nodeId, partitionId);

            if (!groups[index]) {
                continue;
            }

            partitionManagers[nodeId]->addPartition(
                partitionId,
                *groups[index]
            );
        }
    }

    NodeNetwork networkA("127.0.0.1", 9001);
    NodeNetwork networkB("127.0.0.1", 9002);
    NodeNetwork networkC("127.0.0.1", 9003);

    std::array<NodeNetwork*, nodeCount> networks{
        &networkA,
        &networkB,
        &networkC
    };

    // Each local Raft group gets its own network wrapper,
    // election manager, and replication manager.
    std::array<std::unique_ptr<RaftNetwork>, groupCount>
        raftNetworks{};

    std::array<std::unique_ptr<ElectionManager>, groupCount>
        electionManagers{};

    std::array<std::unique_ptr<RaftReplicationManager>, groupCount>
        replicationManagers{};

    for (int nodeId = 0; nodeId < nodeCount; ++nodeId) {
        for (int partitionId = 0;
             partitionId < partitionCount;
             ++partitionId) {

            const int index = groupIndex(nodeId, partitionId);

            if (!groups[index]) {
                continue;
            }

            RaftGroup& group = *groups[index];

            raftNetworks[index] =
                std::make_unique<RaftNetwork>(
                    *networks[nodeId],
                    nodeId,
                    group
                );

            electionManagers[index] =
                std::make_unique<ElectionManager>(
                    group,
                    *raftNetworks[index]
                );

            // Publish successful elections to cluster metadata.
            electionManagers[index]->setLeaderChangeCallback(
                [&metadata](int electedPartitionId, int electedNodeId) {
                    metadata.updateLeader(
                        electedPartitionId,
                        electedNodeId
                    );

                    std::cout
                        << "[ClusterMetadata] Partition "
                        << electedPartitionId
                        << " leader updated to node "
                        << electedNodeId
                        << std::endl;
                }
            );

            replicationManagers[index] =
                std::make_unique<RaftReplicationManager>(
                    group,
                    *raftNetworks[index],
                    *partitionManagers[nodeId]
                );
        }
    }

    RequestServer serverA(*partitionManagers[0], partitionRouter);
    RequestServer serverB(*partitionManagers[1], partitionRouter);
    RequestServer serverC(*partitionManagers[2], partitionRouter);

    std::thread networkThreadA([&networkA]() {
        networkA.start();
    });

    std::thread networkThreadB([&networkB]() {
        networkB.start();
    });

    std::thread networkThreadC([&networkC]() {
        networkC.start();
    });

    for (auto& manager : electionManagers) {
        if (manager) {
            manager->startElectionTimer();
        }
    }

    for (auto& manager : replicationManagers) {
        if (manager) {
            manager->start();
        }
    }

    std::cout << "=============================\n";
    std::cout << "       MDDB SERVER\n";
    std::cout << "=============================\n";
    std::cout << "HTTP API running on:\n";
    std::cout << "http://localhost:8080\n";

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

    for (auto& manager : replicationManagers) {
        if (manager) {
            manager->stop();
        }
    }

    for (auto& manager : electionManagers) {
        if (manager) {
            manager->stopElectionTimer();
        }
    }

    // Preserve the existing network-thread shutdown behavior.
    if (networkThreadA.joinable()) networkThreadA.detach();
    if (networkThreadB.joinable()) networkThreadB.detach();
    if (networkThreadC.joinable()) networkThreadC.detach();

    return 0;
}
