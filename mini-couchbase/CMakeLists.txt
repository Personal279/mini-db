cmake_minimum_required(VERSION 3.16)

project(mini_couchbase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

include_directories(include)

find_package(Crow CONFIG REQUIRED)

include_directories(
    "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include"
)

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

    # Index Client
    src/index/IndexClient.cpp

    # Query
    src/query/QueryExecutor.cpp
    src/query/QueryOptimizer.cpp

    # CLI
    src/cli/CLI.cpp

    # HTTP API
    src/api/HttpServer.cpp
)

target_link_libraries(mini_couchbase PRIVATE
    Crow::Crow
)

# ============================================================
# Codec Test
# ============================================================

add_executable(Codec_test
    tests/Codec_test.cpp
)

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
# Query Optimizer Test
# ============================================================

add_executable(query_optimizer_test
    tests/query_optimizer_test.cpp
    src/query/QueryOptimizer.cpp
)

target_include_directories(query_optimizer_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

# ============================================================
# Network Test
# Requires command-line arguments; run manually.
# ============================================================

add_executable(network_test
    src/network/NetworkTest.cpp

    # Network
    src/network/NodeNetwork.cpp
    src/network/RpcCodec.cpp
)

# ============================================================
# Raft Test
# Requires a node ID; run manually.
# ============================================================

add_executable(raft_test
    src/raft/RaftTest.cpp

    # Network
    src/network/NodeNetwork.cpp
    src/network/RpcCodec.cpp

    # Raft
    src/raft/RaftLog.cpp
    src/raft/RaftNetwork.cpp
    src/raft/RaftReplicationManager.cpp
    src/raft/RaftGroup.cpp
    src/raft/RequestVote.cpp
    src/raft/ElectionManager.cpp
    src/raft/AppendEntries.cpp

    # Node
    src/node/PartitionManager.cpp

    # Index Client
    src/index/IndexClient.cpp

    # Query
    src/query/QueryExecutor.cpp
    src/query/QueryOptimizer.cpp

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
# Partition Manager Test
# ============================================================

add_executable(partition_manager_test
    tests/partition_manager_test.cpp

    # Node
    src/node/PartitionManager.cpp

    # Raft
    src/raft/RaftGroup.cpp
    src/raft/RaftLog.cpp

    # Index Client
    src/index/IndexClient.cpp

    # Query
    src/query/QueryExecutor.cpp
    src/query/QueryOptimizer.cpp

    # Storage
    src/storage/StorageEngine.cpp
    src/storage/WAL.cpp
    src/storage/MemTable.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

target_include_directories(partition_manager_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include"
)

# ============================================================
# Query Executor Test
# Index Scan vs Full Scan
# ============================================================

add_executable(query_executor_test
    tests/query_executor_test.cpp

    # Query
    src/query/QueryOptimizer.cpp
    src/query/QueryExecutor.cpp

    # Index Client
    src/index/IndexClient.cpp

    # Storage
    src/storage/StorageEngine.cpp
    src/storage/WAL.cpp
    src/storage/MemTable.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp
)

target_include_directories(query_executor_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include"
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
# Index Client Test
# ============================================================

add_executable(index_client_test
    tests/index_client_test.cpp
    src/index/IndexClient.cpp
)

target_include_directories(index_client_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

# ============================================================
# Routing Service Test
# ============================================================

add_executable(RoutingService_test
    tests/RoutingService_test.cpp

    # Cluster
    src/cluster/Node.cpp
    src/cluster/Partition.cpp
    src/cluster/ClusterMetadata.cpp

    # Router
    src/router/PartitionRouter.cpp
    src/router/RoutingService.cpp
)

target_include_directories(RoutingService_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

# ============================================================
# Windows Libraries
# ============================================================

if(WIN32)
    target_link_libraries(mini_couchbase PRIVATE
        ws2_32
        winhttp
    )

    target_link_libraries(network_test PRIVATE
        ws2_32
    )

    target_link_libraries(raft_test PRIVATE
        ws2_32
        winhttp
    )

    target_link_libraries(partition_manager_test PRIVATE
        ws2_32
        winhttp
    )

    target_link_libraries(index_client_test PRIVATE
        winhttp
    )

    target_link_libraries(query_executor_test PRIVATE
        ws2_32
        winhttp
    )
endif()

# ============================================================
# Register Automated Tests with CTest
# ============================================================

add_test(
    NAME Codec_test
    COMMAND $<TARGET_FILE:Codec_test>
)

add_test(
    NAME compaction_test
    COMMAND $<TARGET_FILE:compaction_test>
)

add_test(
    NAME index_client_test
    COMMAND $<TARGET_FILE:index_client_test>
)

add_test(
    NAME partition_manager_test
    COMMAND $<TARGET_FILE:partition_manager_test>
)

add_test(
    NAME query_executor_test
    COMMAND $<TARGET_FILE:query_executor_test>
)

add_test(
    NAME query_optimizer_test
    COMMAND $<TARGET_FILE:query_optimizer_test>
)

add_test(
    NAME scan_test
    COMMAND $<TARGET_FILE:scan_test>
)

add_test(
    NAME RoutingService_test
    COMMAND $<TARGET_FILE:RoutingService_test>
)

# network_test and raft_test are intentionally not registered here
# because they require command-line arguments or process orchestration.

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