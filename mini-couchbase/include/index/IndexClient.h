
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>

#include <string>
#include <vector>

struct IndexMetadata {
    std::string name;
    std::string field;
};

class IndexClient {
public:
    explicit IndexClient(
        const std::wstring& host = L"127.0.0.1",
        INTERNET_PORT port = 8090
    );

    std::vector<std::string> lookup(
        const std::string& indexName,
        const std::string& value
    ) const;

    std::vector<IndexMetadata> listIndexes() const;

    void upsertDocument(
        const std::string& documentKey,
        const std::string& jsonDocument
    ) const;

    void deleteDocument(
        const std::string& documentKey
    ) const;

private:
    std::wstring host;
    INTERNET_PORT port;
};
