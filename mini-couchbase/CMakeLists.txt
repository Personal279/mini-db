cmake_minimum_required(VERSION 3.16)

project(mini_couchbase)

set(CMAKE_CXX_STANDARD 17)

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
)