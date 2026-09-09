#include "api/HttpServer.h"

#include <crow.h>
#include <iostream>

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
    StorageEngine& storage
)
    : storage(storage) {}


void HttpServer::start() {

    crow::App<CorsMiddleware> app;


    CROW_ROUTE(
        app,
        "/api/status"
    )
    .methods(crow::HTTPMethod::GET)
    ([&]() {

        crow::json::wvalue response;

        response["status"] = "ONLINE";
        response["database"] = "MDDB";

        return crow::response(response);
    });


    CROW_ROUTE(
        app,
        "/api/put"
    )
    .methods(
        crow::HTTPMethod::POST,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (
            request.method ==
            crow::HTTPMethod::OPTIONS
        ) {
            return crow::response(204);
        }


        auto body =
            crow::json::load(
                request.body
            );


        if (!body) {
            return crow::response(
                400,
                "Invalid JSON"
            );
        }


        if (
            !body.has("key") ||
            !body.has("value")
        ) {
            return crow::response(
                400,
                "Missing key or value"
            );
        }


        std::string key =
            body["key"].s();

        std::string value =
            body["value"].s();


        storage.put(
            key,
            value
        );


        crow::json::wvalue response;

        response["status"] = "OK";
        response["key"] = key;

        return crow::response(response);
    });


    CROW_ROUTE(
        app,
        "/api/get"
    )
    .methods(
        crow::HTTPMethod::GET,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (
            request.method ==
            crow::HTTPMethod::OPTIONS
        ) {
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


        std::string value =
            storage.get(key);


        crow::json::wvalue response;

        response["key"] =
            std::string(key);

        response["value"] =
            value;

        return crow::response(response);
    });


    CROW_ROUTE(
        app,
        "/api/delete"
    )
    .methods(
        crow::HTTPMethod::DELETE,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {

        if (
            request.method ==
            crow::HTTPMethod::OPTIONS
        ) {
            return crow::response(204);
        }


        auto body =
            crow::json::load(
                request.body
            );


        if (
            !body ||
            !body.has("key")
        ) {
            return crow::response(
                400,
                "Missing key"
            );
        }


        std::string key =
            body["key"].s();


        storage.remove(key);


        crow::json::wvalue response;

        response["status"] = "OK";
        response["key"] = key;

        return crow::response(response);
    });


    std::cout
        << "HTTP API running on:"
        << std::endl;

    std::cout
        << "http://localhost:8080"
        << std::endl;


    app.port(8080)
       .multithreaded()
       .run();
}