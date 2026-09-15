#pragma once

#include "raft/RaftGroup.h"
#include "storage/StorageEngine.h"

class RaftStateMachine {
public:
    RaftStateMachine(
        RaftGroup& group,
        StorageEngine& storageEngine
    );

    void applyCommittedEntries();

private:
    RaftGroup& group;
    StorageEngine& storageEngine;

    void applyCommand(
        const std::string& command
    );
};