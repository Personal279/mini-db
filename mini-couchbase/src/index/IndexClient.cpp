#include "index/IndexClient.h"

#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace {

std::wstring widenAscii(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

std::string readResponse(HINTERNET request) {
    std::string response;

    DWORD available = 0;

    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
        std::vector<char> buffer(available);
        DWORD bytesRead = 0;

        if (!WinHttpReadData(
                request,
                buffer.data(),
                available,
                &bytesRead)) {
            throw std::runtime_error("Failed to read HTTP response");
        }

        response.append(buffer.data(), bytesRead);
    }

    return response;
}

std::string sendRequest(
    const std::wstring& host,
    INTERNET_PORT port,
    const std::wstring& method,
    const std::wstring& path,
    const std::string& body
) {
    HINTERNET session = WinHttpOpen(
        L"MiniCouchbase/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) {
        throw std::runtime_error("WinHttpOpen failed");
    }

    HINTERNET connection = WinHttpConnect(
        session,
        host.c_str(),
        port,
        0
    );

    if (!connection) {
        WinHttpCloseHandle(session);
        throw std::runtime_error("WinHttpConnect failed");
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        method.c_str(),
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("WinHttpOpenRequest failed");
    }

    const wchar_t* headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(
        request,
        headers,
        static_cast<DWORD>(-1L),
        body.empty() ? WINHTTP_NO_REQUEST_DATA
                     : const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0
    );

    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("Index Service request failed");
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);

    if (!WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("Failed to read HTTP status");
    }

    std::string response = readResponse(request);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "Index Service returned HTTP " + std::to_string(status)
        );
    }

    return response;
}

std::wstring encodePathSegment(const std::string& value) {
    static const wchar_t hex[] = L"0123456789ABCDEF";
    std::wstring encoded;

    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded.push_back(static_cast<wchar_t>(c));
        } else {
            encoded.push_back(L'%');
            encoded.push_back(hex[c >> 4]);
            encoded.push_back(hex[c & 0x0F]);
        }
    }

    return encoded;
}

}

IndexClient::IndexClient(
    const std::wstring& host,
    INTERNET_PORT port
)
    : host(host), port(port) {
}

std::vector<std::string> IndexClient::lookup(
    const std::string& indexName,
    const std::string& value
) const {
    std::wstring path =
        L"/indexes/" + encodePathSegment(indexName) +
        L"?value=" + encodePathSegment(value);

    std::string response = sendRequest(
        host,
        port,
        L"GET",
        path,
        ""
    );

    std::vector<std::string> keys;

    std::size_t pos = response.find("\"keys\"");
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid Index Service response");
    }

    pos = response.find('[', pos);
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid keys array");
    }

    ++pos;

    while (pos < response.size()) {
        while (pos < response.size() &&
               (response[pos] == ' ' ||
                response[pos] == '\n' ||
                response[pos] == '\r' ||
                response[pos] == '\t' ||
                response[pos] == ',')) {
            ++pos;
        }

        if (pos >= response.size() || response[pos] == ']') {
            break;
        }

        if (response[pos] != '"') {
            throw std::runtime_error("Invalid key in response");
        }

        ++pos;
        std::string key;

        while (pos < response.size() && response[pos] != '"') {
            if (response[pos] == '\\') {
                throw std::runtime_error(
                    "Escaped keys are not supported by this parser"
                );
            }

            key.push_back(response[pos]);
            ++pos;
        }

        if (pos >= response.size()) {
            throw std::runtime_error("Unterminated key in response");
        }

        ++pos;
        keys.push_back(key);
    }

    return keys;
}

void IndexClient::upsertDocument(
    const std::string& documentKey,
    const std::string& jsonDocument
) const {
    sendRequest(
        host,
        port,
        L"PUT",
        L"/documents/" + encodePathSegment(documentKey),
        jsonDocument
    );
}

void IndexClient::deleteDocument(
    const std::string& documentKey
) const {
    sendRequest(
        host,
        port,
        L"DELETE",
        L"/documents/" + encodePathSegment(documentKey),
        ""
    );
}
