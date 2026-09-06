#include "storage/WAL.h"

#include <fstream>
#include <sstream>

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
            break;

        case Operation::DELETE:
            file << "DELETE|"
                 << request.key
                 << "\n";
            break;
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

            requests.push_back(
                Request{
                    Operation::PUT,
                    key,
                    value
                }
            );
        }
        else if (operation == "DELETE") {

            requests.push_back(
                Request{
                    Operation::DELETE,
                    key,
                    ""
                }
            );
        }
    }

    return requests;
}