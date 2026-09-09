#pragma once

#include "storage/StorageEngine.h"

class HttpServer {
public:

    explicit HttpServer(
        StorageEngine& storage
    );

    void start();

private:

    StorageEngine& storage;
};