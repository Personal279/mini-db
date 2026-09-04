#pragma once

class Partition {
public:
    Partition(int id, int activeNodeId);

    int getId() const;
    int getActiveNodeId() const;

private:
    int id;
    int activeNodeId;
};