
#include "index/IndexClient.h"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <sstream>
#include <memory>
#include <utility>

using json = nlohmann::json;

namespace {

struct WinHttpHandle {
    HINTERNET handle = nullptr;

    explicit WinHttpHandle(HINTERNET h = nullptr)
        : handle(h) {}

    ~WinHttpHandle() {
        if (handle) {
            WinHttpCloseHandle(handle);
        }
    }

    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
};

std::wstring utf8ToWide(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        input.data(), static_cast<int>(input.size()),
        nullptr, 0
    );

    if (size <= 0) {
        throw std::runtime_error("Invalid UTF-8 input");
    }

    std::wstring output(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        input.data(), static_cast<int>(input.size()),
        output.data(), size
    );

    return output;
}

std::string wideToUtf8(const std::wstring& input) {
    if (input.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(
        CP_UTF8, 0,
        input.data(), static_cast<int>(input.size()),
        nullptr, 0, nullptr, nullptr
    );

    if (size <= 0) {
        throw std::runtime_error("UTF-8 conversion failed");
    }

    std::string output(size, '\0');

    WideCharToMultiByte(
        CP_UTF8, 0,
        input.data(), static_cast<int>(input.size()),
        output.data(), size, nullptr, nullptr
    );

    return output;
}

std::string urlEncode(const std::string& input) {
    static const char hex[] = "0123456789ABCDEF";

    std::string output;

    for (unsigned char c : input) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            output.push_back(static_cast<char>(c));
        } else {
            output.push_back('%');
            output.push_back(hex[c >> 4]);
            output.push_back(hex[c & 0x0F]);
        }
    }

    return output;
}

std::string sendRequest(
    const std::wstring& host,
    INTERNET_PORT port,
    const std::wstring& method,
    const std::wstring& path,
    const std::string& body = {}
) {
    WinHttpHandle session(
        WinHttpOpen(
            L"MiniCouchbase/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        )
    );

    if (!session.handle) {
        throw std::runtime_error("WinHttpOpen failed");
    }

    WinHttpHandle connection(
        WinHttpConnect(session.handle, host.c_str(), port, 0)
    );

    if (!connection.handle) {
        throw std::runtime_error("WinHttpConnect failed");
    }

    WinHttpHandle request(
        WinHttpOpenRequest(
            connection.handle,
            method.c_str(),
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        )
    );

    if (!request.handle) {
        throw std::runtime_error("WinHttpOpenRequest failed");
    }

    const wchar_t* headers =
        L"Content-Type: application/json\r\n"
        L"Accept: application/json\r\n";

    LPVOID data = body.empty()
        ? WINHTTP_NO_REQUEST_DATA
        : const_cast<char*>(body.data());

    DWORD dataLength = static_cast<DWORD>(body.size());

    BOOL sent = WinHttpSendRequest(
        request.handle,
        headers,
        static_cast<DWORD>(-1L),
        data,
        dataLength,
        dataLength,
        0
    );

    if (!sent) {
        throw std::runtime_error("WinHttpSendRequest failed");
    }

    if (!WinHttpReceiveResponse(request.handle, nullptr)) {
        throw std::runtime_error("WinHttpReceiveResponse failed");
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);

    if (!WinHttpQueryHeaders(
            request.handle,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX)) {
        throw std::runtime_error("Failed to read HTTP status");
    }

    std::string response;

    for (;;) {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(request.handle, &available)) {
            throw std::runtime_error("Failed to query response size");
        }

        if (available == 0) {
            break;
        }

        std::string buffer(available, '\0');
        DWORD bytesRead = 0;

        if (!WinHttpReadData(
                request.handle,
                buffer.data(),
                available,
                &bytesRead)) {
            throw std::runtime_error("Failed to read HTTP response");
        }

        response.append(buffer.data(), bytesRead);
    }

    if (statusCode < 200 || statusCode >= 300) {
        throw std::runtime_error(
            "Index Service HTTP " + std::to_string(statusCode) +
            ": " + response
        );
    }

    return response;
}

} // namespace

IndexClient::IndexClient(
    const std::wstring& host,
    INTERNET_PORT port
) : host(host), port(port) {}

std::vector<IndexMetadata> IndexClient::listIndexes() const {
    const std::string response = sendRequest(
        host, port, L"GET", L"/indexes"
    );

    const json parsed = json::parse(response);

    if (!parsed.is_array()) {
        throw std::runtime_error(
            "GET /indexes returned an unexpected JSON format"
        );
    }

    std::vector<IndexMetadata> result;
    result.reserve(parsed.size());

    for (const auto& item : parsed) {
        if (!item.is_object() ||
            !item.contains("name") ||
            !item["name"].is_string() ||
            !item.contains("field") ||
            !item["field"].is_string()) {
            throw std::runtime_error(
                "Invalid index metadata returned by Index Service"
            );
        }

        result.push_back({
            item["name"].get<std::string>(),
            item["field"].get<std::string>()
        });
    }

    return result;
}

std::vector<std::string> IndexClient::lookup(
    const std::string& indexName,
    const std::string& value
) const {
    const std::wstring path = L"/indexes/" +
        utf8ToWide(urlEncode(indexName)) +
        L"?value=" + utf8ToWide(urlEncode(value));

    const std::string response = sendRequest(
        host, port, L"GET", path
    );

   
    const json parsed = json::parse(response);

    if (!parsed.is_object() ||
        !parsed.contains("keys") ||
        !parsed["keys"].is_array()) {
        throw std::runtime_error(
            "Index lookup returned an unexpected JSON format"
        );
    }

    return parsed["keys"].get<std::vector<std::string>>();

}

void IndexClient::upsertDocument(
    const std::string& documentKey,
    const std::string& jsonDocument
) const {
    const std::wstring path =
        L"/documents/" + utf8ToWide(urlEncode(documentKey));

    // Validate the supplied document JSON before sending it.
    json::parse(jsonDocument);

    sendRequest(
        host, port, L"PUT", path, jsonDocument
    );
}

void IndexClient::deleteDocument(
    const std::string& documentKey
) const {
    const std::wstring path =
        L"/documents/" + utf8ToWide(urlEncode(documentKey));

    sendRequest(
        host, port, L"DELETE", path
    );
}
