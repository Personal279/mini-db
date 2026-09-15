#include "raft/RaftStateMachine.h"

#include <sstream>
#include <stdexcept>
#include <string>

RaftStateMachine::RaftStateMachine(
    RaftGroup& group,
    StorageEngine& storageEngine
)
    : group(group),
      storageEngine(storageEngine) {
}

void RaftStateMachine::applyCommittedEntries() {
    int commitIndex =
        group.getCommitIndex();

    int lastApplied =
        group.getLastApplied();

    while (lastApplied < commitIndex) {
        int nextIndex =
            lastApplied + 1;

        const LogEntry* entry =
            group.getLog().get(nextIndex);

        if (entry == nullptr) {
            break;
        }

        applyCommand(entry->command);

        group.setLastApplied(nextIndex);

        lastApplied = nextIndex;
    }
}

void RaftStateMachine::applyCommand(
    const std::string& command
) {
    std::stringstream stream(command);

    std::string operation;
    std::string key;
    std::string value;

    std::getline(
        stream,
        operation,
        '|'
    );

    std::getline(
        stream,
        key,
        '|'
    );

    std::getline(
        stream,
        value
    );

    if (operation == "PUT") {
        storageEngine.put(
            key,
            value
        );

        return;
    }

    if (operation == "DELETE") {
        storageEngine.remove(key);

        return;
    }

    if (operation == "GET") {
        storageEngine.get(key);
        return;
    }

    throw std::runtime_error(
        "Unknown Raft command: " + command
    );
}