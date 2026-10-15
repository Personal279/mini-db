
cmake_minimum_required(VERSION 3.16)

project(mini_couchbase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

find_package(Crow CONFIG REQUIRED)

# ============================================================
# Main Mini Couchbase Executable
# ============================================================

add_executable(mini_couchbase
    src/main.cpp

    # Client
    src/client/Client.cpp

    # Cluster
    src/cluster/Node.cpp
    src/cluster/Partition.cpp
    src/cluster/ClusterMetadata.cpp

    # Node
    src/node/RequestServer.cpp
    src/node/PartitionManager.cpp

    # Router
    src/router/PartitionRouter.cpp
    src/router/RoutingService.cpp

    # Network
    src/network/NodeNetwork.cpp
    src/network/RpcCodec.cpp

    # Raft
    src/raft/RaftLog.cpp
    src/raft/RaftGroup.cpp
    src/raft/RequestVote.cpp
    src/raft/ElectionManager.cpp
    src/raft/RaftNetwork.cpp
    src/raft/AppendEntries.cpp
    src/raft/RaftReplicationManager.cpp

    # Storage
    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp

    # CLI
    src/cli/CLI.cpp

    # HTTP API
    src/api/HttpServer.cpp
)

target_link_libraries(mini_couchbase PRIVATE Crow::Crow)

# ============================================================
# Benchmark
# ============================================================

add_executable(mddb_benchmark
    src/benchmark/BenchMark.cpp

    # Storage
    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

# ============================================================
# Network Test
# ============================================================

add_executable(network_test
    src/network/NetworkTest.cpp

    # Network
    src/network/NodeNetwork.cpp
    src/network/RpcCodec.cpp
)

# ============================================================
# Raft Test
# ============================================================

add_executable(raft_test
    src/raft/RaftTest.cpp

    # Network
    src/network/NodeNetwork.cpp
    src/network/RpcCodec.cpp

    # Raft
    src/raft/RaftLog.cpp
    src/raft/RaftReplicationManager.cpp
    src/raft/RaftGroup.cpp
    src/raft/RequestVote.cpp
    src/raft/ElectionManager.cpp
    src/raft/RaftNetwork.cpp
    src/raft/AppendEntries.cpp

    # Node
    src/node/PartitionManager.cpp

    # Storage
    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

# ============================================================
# Full-Scan Test
# ============================================================

add_executable(scan_test
    tests/scan_test.cpp

    # Storage
    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

# ============================================================
# Compaction Test
# ============================================================

add_executable(compaction_test
    tests/storage/CompactionTest.cpp

    # Storage
    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

# ============================================================
# Windows Libraries
# ============================================================

if(WIN32)
    target_link_libraries(mini_couchbase PRIVATE ws2_32)
    target_link_libraries(network_test PRIVATE ws2_32)
    target_link_libraries(raft_test PRIVATE ws2_32)
endif()

# ============================================================
# Installation
# ============================================================

install(
    TARGETS mini_couchbase
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY data/
    DESTINATION bin/data
)

install(
    DIRECTORY UI/PestDB/dist/
    DESTINATION UI
)

# ============================================================
# CPack Installer
# ============================================================

set(CPACK_PACKAGE_NAME "Mini-Couchbase")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Mini-Couchbase")

set(
    CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Mini Couchbase Distributed Key-Value Database"
)

set(CPACK_GENERATOR "NSIS")

include(CPack)
