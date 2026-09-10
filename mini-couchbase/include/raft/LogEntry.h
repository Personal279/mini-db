#pragma once

#include <string>

struct LogEntry {
    int term;
    int index;
    std::string command;
};