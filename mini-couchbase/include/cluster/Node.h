#pragma once

#include <string>

class Node {
public:
    Node(int id, const std::string& address);

    int getId() const;
    const std::string& getAddress() const;

private:
    int id;
    std::string address;
};