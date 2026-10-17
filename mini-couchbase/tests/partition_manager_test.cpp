
#include "node/PartitionManager.h"

#include <cassert>
#include <filesystem>
#include <iostream>

#include "raft/RaftGroup.h"
#include "router/PartitionRouter.h"
#include "storage/StorageEngine.h"

int main() {
    namespace fs = std::filesystem;

    const fs::path testDir =
        fs::temp_directory_path() / "mddb_partition_manager_test";

    fs::remove_all(testDir);
    fs::create_directories(testDir);

    const fs::path walPath = testDir / "test.wal";
    const fs::path dataDir = testDir / "data";

    {
        StorageEngine storage(
            walPath.string(),
            dataDir.string(),
            100
        );

        PartitionRouter partitionRouter(4);

        // Create a separate Raft group for each partition replica.
        RaftGroup raftGroup0(1, 0, {2, 3});
        RaftGroup raftGroup2(1, 2, {2, 3});

        PartitionManager manager(storage, partitionRouter);

        // No partitions have been assigned yet.
        assert(!manager.ownsPartition(0));
        assert(!manager.ownsPartition(1));
        assert(!manager.ownsPartition(10));

        // Add the first partition.
        manager.addPartition(0, raftGroup0);

        assert(manager.ownsPartition(0));
        assert(!manager.ownsPartition(1));

        // Add another partition.
        manager.addPartition(2, raftGroup2);

        assert(manager.ownsPartition(0));
        assert(manager.ownsPartition(2));
        assert(!manager.ownsPartition(1));

        // Repeated ownership checks should remain consistent.
        for (int i = 0; i < 1000; ++i) {
            assert(manager.ownsPartition(0));
            assert(manager.ownsPartition(2));
            assert(!manager.ownsPartition(1));
        }
    }

    fs::remove_all(testDir);

    std::cout << "All partition ownership tests passed.\n";
    return 0;
}
