#pragma once

#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "common/Types.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef DELETE
#endif

class WAL {
public:
    explicit WAL(const std::string& filePath,
                 std::chrono::milliseconds syncInterval = std::chrono::milliseconds(5),
                 std::size_t syncBatchSize = 256);
    ~WAL();

    void append(const Request& request);

    std::vector<Request> recover();

private:
    void writerLoop();
    static std::string serialize(const Request& request);

    std::string filePath;

#ifdef _WIN32
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
#else
    int fileDescriptor = -1;
#endif

    std::thread writerThread;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::deque<Request> pendingRequests;

    std::atomic<bool> shutdownRequested{false};

    std::chrono::milliseconds syncInterval;
    std::size_t syncBatchSize;
};