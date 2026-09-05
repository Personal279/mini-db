#include "node/PartitionManager.h"

#include <iostream>

PartitionManager::PartitionManager(
    StorageEngine& storageEngine
)
    : storageEngine(storageEngine) {
}

void PartitionManager::addPartition(int partitionId) {
    partitions.push_back(partitionId);
}

bool PartitionManager::ownsPartition(int partitionId) const {
    for (int id : partitions) {
        if (id == partitionId) {
            return true;
        }
    }

    return false;
}

void PartitionManager::processRequest(
    const Request& request
) {
    switch (request.operation) {

        case Operation::PUT:
            storageEngine.put(
                request.key,
                request.value
            );

            std::cout << "PUT completed: "
                      << request.key << std::endl;
            break;


        case Operation::GET: {
            std::string value =
                storageEngine.get(request.key);

            std::cout << "GET result: "
                      << value << std::endl;
            break;
        }


        case Operation::DELETE:
            storageEngine.remove(request.key);

            std::cout << "DELETE completed: "
                      << request.key << std::endl;
            break;
    }
}