// api/HttpServer.cpp

#include "api/HttpServer.h"

#include <crow.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

struct CorsMiddleware {

    struct context {};

    void before_handle(
        crow::request&,
        crow::response&,
        context&
    ) {}

    template <typename AllContext>
    void after_handle(
        crow::request const&,
        crow::response& response,
        context&,
        AllContext&
    ) {
        response.add_header(
            "Access-Control-Allow-Origin",
            "*"
        );

        response.add_header(
            "Access-Control-Allow-Methods",
            "GET, POST, DELETE, OPTIONS"
        );

        response.add_header(
            "Access-Control-Allow-Headers",
            "Content-Type"
        );
    }
};


HttpServer::HttpServer(
    std::vector<std::reference_wrapper<RequestServer>> servers
)
    : servers(std::move(servers)) {}


Response HttpServer::dispatch(const Request& request) {

    Response last{
        false,
        false,
        "",
        "No node available"
    };

    for (auto& serverRef : servers) {

        Response response =
            serverRef.get().handleRequest(request);

        if (response.success) {
            return response;
        }

        last = response;
    }

    return last;
}


void HttpServer::start() {

    crow::App<CorsMiddleware> app;


    // ============================================================
    // Frontend UI
    // ============================================================

    CROW_ROUTE(app, "/")
    .methods(crow::HTTPMethod::GET)
    ([&]() {

        std::ifstream file(
            "../UI/index.html",
            std::ios::binary
        );

        if (!file.is_open()) {
            return crow::response(
                500,
                "UI index.html not found"
            );
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        crow::response response(buffer.str());

        response.set_header(
            "Content-Type",
            "text/html"
        );

        return response;
    });


    // ============================================================
    // Frontend Assets
    // ============================================================

    CROW_ROUTE(app, "/assets/<string>")
    .methods(crow::HTTPMethod::GET)
    ([&](const std::string& filename) {

        std::ifstream file(
            "../UI/assets/" + filename,
            std::ios::binary
        );

        if (!file.is_open()) {
            return crow::response(
                404,
                "Asset not found"
            );
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        crow::response response(buffer.str());

        response.set_header(
            "Content-Type",
            "application/javascript"
        );

        return response;
    });


    // ============================================================
    // API - Status
    // ============================================================

    CROW_ROUTE(app, "/api/status")
    .methods(crow::HTTPMethod::GET)
    ([&]() {

        crow::json::wvalue response;

        response["status"] = "ONLINE";
        response["database"] = "MDDB";

        return crow::response(response);
    });


    // ============================================================
    // API - PUT
    // ============================================================

    CROW_ROUTE(app, "/api/put")
    .methods(
        crow::HTTPMethod::POST,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (request.method == crow::HTTPMethod::OPTIONS) {
            return crow::response(204);
        }

        auto body = crow::json::load(request.body);

        if (!body) {
            return crow::response(
                400,
                "Invalid JSON"
            );
        }

        if (!body.has("key") || !body.has("value")) {
            return crow::response(
                400,
                "Missing key or value"
            );
        }

        Request req;

        req.operation = Operation::PUT;
        req.key = body["key"].s();
        req.value = body["value"].s();

        Response result = dispatch(req);

        crow::json::wvalue response;

        response["status"] =
            result.success ? "OK" : "ERROR";

        response["message"] =
            result.message;

        response["key"] =
            req.key;

        return crow::response(
            result.success ? 200 : 409,
            response
        );
    });


    // ============================================================
    // API - GET
    // ============================================================

    CROW_ROUTE(app, "/api/get")
    .methods(
        crow::HTTPMethod::GET,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (request.method == crow::HTTPMethod::OPTIONS) {
            return crow::response(204);
        }

        auto key =
            request.url_params.get("key");

        if (!key) {
            return crow::response(
                400,
                "Missing key"
            );
        }

        Request req;

        req.operation = Operation::GET;
        req.key = key;

        Response result = dispatch(req);

        crow::json::wvalue response;

        response["key"] =
            std::string(key);

        response["value"] =
            result.value;

        return crow::response(response);
    });


    // ============================================================
    // API - DELETE
    // ============================================================

    CROW_ROUTE(app, "/api/delete")
    .methods(
        crow::HTTPMethod::DELETE,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (request.method == crow::HTTPMethod::OPTIONS) {
            return crow::response(204);
        }

        auto body =
            crow::json::load(request.body);

        if (!body || !body.has("key")) {
            return crow::response(
                400,
                "Missing key"
            );
        }

        Request req;

        req.operation = Operation::DELETE;
        req.key = body["key"].s();

        Response result = dispatch(req);

        crow::json::wvalue response;

        response["status"] =
            result.success ? "OK" : "ERROR";

        response["message"] =
            result.message;

        response["key"] =
            req.key;

        return crow::response(
            result.success ? 200 : 409,
            response
        );
    });


    // ============================================================
    // Start Server
    // ============================================================

    std::cout << "HTTP API running on:" << std::endl;
    std::cout << "http://localhost:8080" << std::endl;

    app
        .port(8080)
        .multithreaded()
        .run();
}