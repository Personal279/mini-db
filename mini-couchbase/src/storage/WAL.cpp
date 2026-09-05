#include "storage/WAL.h"

#include <fstream>

WAL::WAL(const std::string& filePath)
    : filePath(filePath) {
}

void WAL::append(const Request& request) {
    std::ofstream file(
        filePath,
        std::ios::app
    );

    if (!file.is_open()) {
        return;
    }

    switch (request.operation) {

        case Operation::PUT:
            file << "PUT|"
                 << request.key
                 << "|"
                 << request.value
                 << "\n";
            break;

        case Operation::GET:
            // GET is not written to WAL
            break;

        case Operation::DELETE:
            file << "DELETE|"
                 << request.key
                 << "\n";
            break;
    }

    file.close();
}