#include "index/IndexClient.h"
#include <iostream>

int main() {
    try {
        IndexClient client;

        for (const auto& index : client.listIndexes()) {
            std::cout << "Index: " << index.name
                      << ", field: " << index.field << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}