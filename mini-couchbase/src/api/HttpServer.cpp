// api/HttpServer.cpp

#include "api/HttpServer.h"
#include "query/QueryExecutor.h"
#include <nlohmann/json.hpp>
#include <crow.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <exception>

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
    std::vector<std::reference_wrapper<RequestServer>> servers,
    RoutingService& routingService,
    std::vector<Node> nodes
)
    : servers(std::move(servers)),
      routingService(routingService),
      nodes(std::move(nodes)) {}

Response HttpServer::dispatch(const Request& request) {
    try {
        const int leaderNodeId = routingService.route(request);

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].getId() == leaderNodeId) {
                if (i < servers.size()) {
                    return servers[i].get().handleRequest(request);
                }
                break;
            }
        }

        return {false, false, "", "Leader request server not found"};
    } catch (const std::exception& e) {
        return {false, false, "", e.what()};
    }
}


void HttpServer::start() {



    crow::App<CorsMiddleware> app;




    // ============================================================
    // API - QUERY
    // ============================================================

    CROW_ROUTE(app, "/api/query")
    .methods(
        crow::HTTPMethod::POST,
        crow::HTTPMethod::OPTIONS
    )
    ([&](const crow::request& request) {
        if (request.method == crow::HTTPMethod::OPTIONS) {
            return crow::response(204);
        }

        try {
            const auto body =
                nlohmann::json::parse(request.body);

            if (!body.is_object() ||
                !body.contains("field") ||
                !body["field"].is_string() ||
                !body.contains("value") ||
                !body["value"].is_string()) {
                return crow::response(
                    400,
                    "Expected string fields: field and value"
                );
            }

            if (servers.empty()) {
                return crow::response(
                    503,
                    "No request server available"
                );
            }

            QueryPredicate predicate{
                body["field"].get<std::string>(),
                body["value"].get<std::string>()
            };

            // Execute through the existing request-server layer.
            QueryResult result =
                servers.front().get().executeQuery(predicate);

            nlohmann::json response;
            response["plan"] =
                result.plan.type == PlanType::IndexScan
                    ? "IndexScan"
                    : "FullScan";

            response["index"] = result.plan.indexName;
            response["estimated_cost"] = result.plan.estimatedCost;
            response["documents"] = nlohmann::json::array();

            for (const auto& [key, document] : result.documents) {
                nlohmann::json item;
                item["key"] = key;

                try {
                    item["value"] = nlohmann::json::parse(document);
                } catch (const nlohmann::json::exception&) {
                    item["value"] = document;
                }

                response["documents"].push_back(std::move(item));
            }

            crow::response httpResponse(200, response.dump());
            httpResponse.set_header(
                "Content-Type",
                "application/json"
            );

            return httpResponse;

        } catch (const nlohmann::json::exception& error) {
            return crow::response(
                400,
                std::string("Invalid query JSON: ") + error.what()
            );
        } catch (const std::exception& error) {
            return crow::response(
                500,
                std::string("Query execution failed: ") + error.what()
            );
        }
    });

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

        // No leader answered (for example during an election). Report it
        // instead of returning an empty value with HTTP 200.
        if (!result.success) {
            crow::json::wvalue error;

            error["status"] = "ERROR";
            error["message"] = result.message;
            error["key"] = std::string(key);

            return crow::response(503, error);
        }

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