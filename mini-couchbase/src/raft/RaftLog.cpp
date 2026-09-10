#include "raft/RaftLog.h"

RaftLog::RaftLog() {
}

int RaftLog::getLastIndex() const {
    if (log.empty()) {
        return 0;
    }

    return log.back().index;
}

int RaftLog::getLastTerm() const {
    if (log.empty()) {
        return 0;
    }

    return log.back().term;
}

void RaftLog::append(
    const LogEntry& entry
) {
    log.push_back(entry);
}

const LogEntry* RaftLog::get(
    int index
) const {
    for (const auto& entry : log) {
        if (entry.index == index) {
            return &entry;
        }
    }

    return nullptr;
}

void RaftLog::removeFrom(
    int index
) {
    while (
        !log.empty() &&
        log.back().index >= index
    ) {
        log.pop_back();
    }
}

const std::vector<LogEntry>& RaftLog::entries() const {
    return log;
}