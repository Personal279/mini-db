cmake_minimum_required(VERSION 3.16)

project(mini_couchbase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)


add_executable(mini_couchbase

    src/main.cpp

    src/client/Client.cpp

    src/router/RequestRouter.cpp

    src/cluster/Node.cpp
    src/cluster/Partition.cpp
    src/cluster/ClusterMetadata.cpp

    src/node/RequestServer.cpp
    src/node/PartitionManager.cpp

    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp

    src/cli/CLI.cpp
)


add_executable(mddb_benchmark

    src/benchmark/BenchMark.cpp

    src/storage/MemTable.cpp
    src/storage/WAL.cpp
    src/storage/StorageEngine.cpp
    src/storage/SSTable.cpp
    src/storage/BloomFilter.cpp

)