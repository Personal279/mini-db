#include "index/IndexClient.h"

#include <iostream>
#include <string>

int main() {
    try {
        IndexClient client;

        client.upsertDocument(
            "user:102",
            R"({"name":"TestUser","city":"Mysuru"})"
        );

        auto keys = client.lookup("city_idx", "Mysuru");

        std::cout << "Keys for Mysuru:\n";
        for (const auto& key : keys) {
            std::cout << key << '\n';
        }

        client.deleteDocument("user:102");

        auto deletedKeys = client.lookup("city_idx", "Mysuru");

        std::cout << "Keys after deletion:\n";
        for (const auto& key : deletedKeys) {
            std::cout << key << '\n';
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }
}