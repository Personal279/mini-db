#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct LogEntry { int term; int index; std::string command; };
struct Req {
    int term; int leaderId; int prevLogIndex; int prevLogTerm;
    std::vector<LogEntry> entries; int leaderCommit;
};

// ---------- OLD (copied from RaftNetwork.cpp) ----------
static std::string encodeOld(const Req& request) {
    std::stringstream stream;
    stream << request.term << "|" << request.leaderId << "|"
           << request.prevLogIndex << "|" << request.prevLogTerm << "|"
           << request.leaderCommit << "|" << request.entries.size();
    for (const LogEntry& entry : request.entries) {
        stream << "|" << entry.term << "|" << entry.index << "|" << entry.command;
    }
    return stream.str();
}

static Req decodeOld(const std::string& payload) {
    std::stringstream stream(payload);
    std::string value;
    std::getline(stream, value, '|'); int term = std::stoi(value);
    std::getline(stream, value, '|'); int leaderId = std::stoi(value);
    std::getline(stream, value, '|'); int prevLogIndex = std::stoi(value);
    std::getline(stream, value, '|'); int prevLogTerm = std::stoi(value);
    std::getline(stream, value, '|'); int leaderCommit = std::stoi(value);
    std::getline(stream, value, '|'); int entryCount = std::stoi(value);
    std::vector<LogEntry> entries;
    for (int i = 0; i < entryCount; i++) {
        std::string entryTerm, entryIndex, command;
        std::getline(stream, entryTerm, '|');
        std::getline(stream, entryIndex, '|');
        if (i + 1 == entryCount) std::getline(stream, command);
        else std::getline(stream, command, '|');
        entries.push_back({std::stoi(entryTerm), std::stoi(entryIndex), command});
    }
    return {term, leaderId, prevLogIndex, prevLogTerm, entries, leaderCommit};
}

// ---------- NEW (length-prefixed command) ----------
static std::string encodeNew(const Req& request) {
    std::stringstream stream;
    stream << request.term << "|" << request.leaderId << "|"
           << request.prevLogIndex << "|" << request.prevLogTerm << "|"
           << request.leaderCommit << "|" << request.entries.size();
    for (const LogEntry& entry : request.entries) {
        stream << "|" << entry.term << "|" << entry.index << "|"
               << entry.command.size() << "|" << entry.command;
    }
    return stream.str();
}

static Req decodeNew(const std::string& payload) {
    std::stringstream stream(payload);
    std::string value;
    std::getline(stream, value, '|'); int term = std::stoi(value);
    std::getline(stream, value, '|'); int leaderId = std::stoi(value);
    std::getline(stream, value, '|'); int prevLogIndex = std::stoi(value);
    std::getline(stream, value, '|'); int prevLogTerm = std::stoi(value);
    std::getline(stream, value, '|'); int leaderCommit = std::stoi(value);
    std::getline(stream, value, '|'); int entryCount = std::stoi(value);
    std::vector<LogEntry> entries;
    for (int i = 0; i < entryCount; i++) {
        std::string entryTerm, entryIndex, commandLength;
        std::getline(stream, entryTerm, '|');
        std::getline(stream, entryIndex, '|');
        std::getline(stream, commandLength, '|');
        std::size_t length = std::stoul(commandLength);
        std::string command(length, '\0');
        stream.read(&command[0], static_cast<std::streamsize>(length));
        if (static_cast<std::size_t>(stream.gcount()) != length)
            throw std::runtime_error("AppendEntries payload truncated");
        if (i + 1 < entryCount) stream.ignore(1);
        entries.push_back({std::stoi(entryTerm), std::stoi(entryIndex), command});
    }
    return {term, leaderId, prevLogIndex, prevLogTerm, entries, leaderCommit};
}

static Req makeReq(int n, bool weird = false) {
    Req r{1, 0, 0, 0, {}, 0};
    for (int i = 1; i <= n; i++) {
        std::string cmd = "PUT|rotate:" + std::to_string(i) + "|v" + std::to_string(i);
        if (weird) cmd = "PUT|k,|" + std::to_string(i) + "|a|b,c:d\ne";
        r.entries.push_back({1, i, cmd});
    }
    return r;
}

static bool same(const Req& a, const Req& b) {
    if (a.entries.size() != b.entries.size()) return false;
    for (size_t i = 0; i < a.entries.size(); i++) {
        if (a.entries[i].term != b.entries[i].term ||
            a.entries[i].index != b.entries[i].index ||
            a.entries[i].command != b.entries[i].command) return false;
    }
    return true;
}

template <typename Dec>
static void run(const char* label, Dec dec, const std::string& payload, const Req& expected) {
    try {
        Req got = dec(payload);
        std::cout << label << ": decoded " << got.entries.size()
                  << " entries, roundtrip " << (same(got, expected) ? "EQUAL" : "MISMATCH") << "\n";
    } catch (const std::exception& e) {
        std::cout << label << ": THROWS " << e.what() << "\n";
    }
}

int main() {
    Req one = makeReq(1), seven = makeReq(7), weird = makeReq(3, true);

    std::cout << "== OLD codec ==\n";
    run("old entries=1", decodeOld, encodeOld(one), one);
    run("old entries=7", decodeOld, encodeOld(seven), seven);

    std::cout << "== NEW codec ==\n";
    run("new entries=1", decodeNew, encodeNew(one), one);
    run("new entries=7", decodeNew, encodeNew(seven), seven);
    run("new weird cmds (|,:,\\n)", decodeNew, encodeNew(weird), weird);

    std::cout << "== Truncation at 4095 bytes (recv buffer) ==\n";
    Req big = makeReq(400);
    std::string p = encodeNew(big);
    std::cout << "payload size for 400 entries: " << p.size() << "\n";
    run("new truncated", decodeNew, p.substr(0, 4095), big);
    run("old truncated", decodeOld, encodeOld(big).substr(0, 4095), big);
    return 0;
}