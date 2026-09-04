#include "cluster/Node.h"

Node::Node(int id, const std::string& address)
    : id(id), address(address) {
}

int Node::getId() const {
    return id;
}

const std::string& Node::getAddress() const {
    return address;
}