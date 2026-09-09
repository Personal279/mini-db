#include "storage/WAL.h"

#include <fstream>
#include <sstream>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

WAL::WAL(const std::string& filePath,
         std::chrono::milliseconds syncInterval,
         std::size_t syncBatchSize)
    : filePath(filePath),
      syncInterval(syncInterval),
      syncBatchSize(syncBatchSize) {

#ifdef _WIN32
    fileHandle = ::CreateFileA(
        filePath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (fileHandle != INVALID_HANDLE_VALUE) {
        ::SetFilePointer(fileHandle, 0, nullptr, FILE_END);
    }
#else
    fileDescriptor = ::open(filePath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
#endif

    writerThread = std::thread(&WAL::writerLoop, this);
}

WAL::~WAL() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        shutdownRequested = true;
    }

    queueCondition.notify_one();

    if (writerThread.joinable()) {
        writerThread.join();
    }

#ifdef _WIN32
    if (fileHandle != INVALID_HANDLE_VALUE) {
        ::FlushFileBuffers(fileHandle);
        ::CloseHandle(fileHandle);
    }
#else
    if (fileDescriptor >= 0) {
        ::fsync(fileDescriptor);
        ::close(fileDescriptor);
    }
#endif
}

std::string WAL::serialize(const Request& request) {
    std::string line;

    switch (request.operation) {
        case Operation::PUT:
            line.reserve(request.key.size() + request.value.size() + 8);
            line += "PUT|";
            line += request.key;
            line += "|";
            line += request.value;
            line += "\n";
            break;

        case Operation::DELETE:
            line.reserve(request.key.size() + 12);
            line += "DELETE|";
            line += request.key;
            line += "\n";
            break;

        case Operation::GET:
            break;
    }

    return line;
}

void WAL::append(const Request& request) {
    std::lock_guard<std::mutex> lock(queueMutex);
    pendingRequests.push_back(request);
    queueCondition.notify_one();
}

void WAL::writerLoop() {
    while (true) {
        std::deque<Request> batch;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            queueCondition.wait_for(lock, syncInterval, [this] {
                return shutdownRequested || pendingRequests.size() >= syncBatchSize;
            });

            if (pendingRequests.empty() && shutdownRequested) {
                break;
            }

            batch.swap(pendingRequests);
        }

        if (batch.empty()) {
            continue;
        }

        std::string buffer;
        for (const auto& request : batch) {
            buffer += serialize(request);
        }

        if (buffer.empty()) {
            continue;
        }

#ifdef _WIN32
        if (fileHandle != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten = 0;
            ::WriteFile(fileHandle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesWritten, nullptr);
            ::FlushFileBuffers(fileHandle);
        }
#else
        if (fileDescriptor >= 0) {
            ::write(fileDescriptor, buffer.data(), buffer.size());
            ::fsync(fileDescriptor);
        }
#endif
    }
}

std::vector<Request> WAL::recover() {
    std::vector<Request> requests;

    std::ifstream file(filePath);

    if (!file.is_open()) {
        return requests;
    }

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream stream(line);

        std::string operation;
        std::string key;
        std::string value;

        std::getline(stream, operation, '|');
        std::getline(stream, key, '|');
        std::getline(stream, value);

        if (operation == "PUT") {
            requests.push_back(Request{Operation::PUT, key, value});
        } else if (operation == "DELETE") {
            requests.push_back(Request{Operation::DELETE, key, ""});
        }
    }

    return requests;
}