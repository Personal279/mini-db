#pragma once

#include <vector>

#include "raft/LogEntry.h"

class RaftLog {
public:
    RaftLog();

    int getLastIndex() const;

    int getLastTerm() const;

    void append(
        const LogEntry& entry
    );

    const LogEntry* get(
        int index
    ) const;

    void removeFrom(
        int index
    );

    const std::vector<LogEntry>& entries() const;

private:
    std::vector<LogEntry> log;
};