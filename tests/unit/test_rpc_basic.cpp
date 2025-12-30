/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for basic RPC patterns in Thallium
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

TEST_SUITE("RPC Basic") {

TEST_CASE("rpc define client side") {
    tl::engine client("tcp", THALLIUM_CLIENT_MODE);

    // Client should be able to define RPC without handler
    REQUIRE_NOTHROW({
        tl::remote_procedure rpc = client.define("test_rpc");
    });

    client.finalize();
}

TEST_CASE("rpc define server side") {
    tl::engine server("tcp", THALLIUM_SERVER_MODE, true);

    // Server should be able to define RPC with handler
    REQUIRE_NOTHROW({
        server.define("test_rpc", [](const tl::request& req) {
            req.respond();
        });
    });

    server.finalize();
}

TEST_CASE("rpc call no args no return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    bool handler_called = false;

    myEngine.define("hello", [&](const tl::request& req) {
        handler_called = true;
        req.respond();
    });

    auto rpc = myEngine.define("hello");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)();

    REQUIRE(handler_called == true);
    myEngine.finalize();
}

TEST_CASE("rpc call with args no return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    int received_value = 0;

    myEngine.define("set_value", [&](const tl::request& req, int val) {
        received_value = val;
        req.respond();
    });

    auto rpc = myEngine.define("set_value");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)(42);

    REQUIRE(received_value == 42);
    myEngine.finalize();
}

TEST_CASE("rpc call no args with return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("get_value", [](const tl::request& req) {
        req.respond(42);
    });

    auto rpc = myEngine.define("get_value");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)();

    REQUIRE(result == 42);
    myEngine.finalize();
}

TEST_CASE("rpc call with args with return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("add", [](const tl::request& req, int a, int b) {
        req.respond(a + b);
    });

    auto rpc = myEngine.define("add");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(5, 7);

    REQUIRE(result == 12);
    myEngine.finalize();
}

TEST_CASE("rpc call multiple args") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("multiply_and_add", [](const tl::request& req, int a, int b, int c) {
        req.respond(a * b + c);
    });

    auto rpc = myEngine.define("multiply_and_add");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(3, 4, 5);

    REQUIRE(result == 17);  // 3*4 + 5 = 17
    myEngine.finalize();
}

TEST_CASE("rpc disable response") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("fire_and_forget", [](const tl::request& req, int val) {
        // Handler executes but doesn't send response
    }).disable_response();

    auto rpc = myEngine.define("fire_and_forget").disable_response();
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Should complete without waiting for response
    REQUIRE_NOTHROW(rpc.on(self_ep)(42));

    myEngine.finalize();
}

TEST_CASE("rpc lambda handler") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("lambda_rpc", [](const tl::request& req, int x) {
        req.respond(x * 2);
    });

    auto rpc = myEngine.define("lambda_rpc");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(21);

    REQUIRE(result == 42);
    myEngine.finalize();
}

// Function for testing function pointer handler
static void function_handler(const tl::request& req, int x) {
    req.respond(x * 3);
}

TEST_CASE("rpc function pointer handler") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("func_ptr_rpc", function_handler);

    auto rpc = myEngine.define("func_ptr_rpc");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(14);

    REQUIRE(result == 42);
    myEngine.finalize();
}

TEST_CASE("rpc std function handler") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::function<void(const tl::request&, int)> handler =
        [](const tl::request& req, int x) {
            req.respond(x / 2);
        };

    myEngine.define("std_func_rpc", handler);

    auto rpc = myEngine.define("std_func_rpc");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(84);

    REQUIRE(result == 42);
    myEngine.finalize();
}

TEST_CASE("rpc multiple sequential calls") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::atomic<int> call_count{0};

    myEngine.define("increment", [&](const tl::request& req) {
        int count = call_count.fetch_add(1) + 1;
        req.respond(count);
    });

    auto rpc = myEngine.define("increment");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result1 = rpc.on(self_ep)();
    int result2 = rpc.on(self_ep)();
    int result3 = rpc.on(self_ep)();

    REQUIRE(result1 == 1);
    REQUIRE(result2 == 2);
    REQUIRE(result3 == 3);
    myEngine.finalize();
}

TEST_CASE("rpc call on endpoint") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo", [](const tl::request& req, int val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)(123);

    REQUIRE(result == 123);
    myEngine.finalize();
}

TEST_CASE("rpc with string arguments") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("concat", [](const tl::request& req,
                                const std::string& a, const std::string& b) {
        req.respond(a + b);
    });

    auto rpc = myEngine.define("concat");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::string result = rpc.on(self_ep)(std::string("Hello, "), std::string("World!"));

    REQUIRE(result == "Hello, World!");
    myEngine.finalize();
}

TEST_CASE("rpc with mixed argument types") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("mixed", [](const tl::request& req, int i, double d, const std::string& s) {
        std::string result = s + " " + std::to_string(i) + " " + std::to_string(d);
        req.respond(result);
    });

    auto rpc = myEngine.define("mixed");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::string result = rpc.on(self_ep)(42, 3.14, std::string("test"));

    REQUIRE(result.find("test") != std::string::npos);
    REQUIRE(result.find("42") != std::string::npos);
    myEngine.finalize();
}

TEST_CASE("rpc request get endpoint") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::string caller_address;

    myEngine.define("who_called", [&](const tl::request& req) {
        tl::endpoint caller = req.get_endpoint();
        caller_address = static_cast<std::string>(caller);
        req.respond();
    });

    auto rpc = myEngine.define("who_called");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)();

    // Caller address should be set
    REQUIRE(!caller_address.empty());
    myEngine.finalize();
}

TEST_CASE("rpc unique names") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Define multiple RPCs with different names
    myEngine.define("rpc1", [](const tl::request& req) { req.respond(1); });
    myEngine.define("rpc2", [](const tl::request& req) { req.respond(2); });
    myEngine.define("rpc3", [](const tl::request& req) { req.respond(3); });

    auto rpc1 = myEngine.define("rpc1");
    auto rpc2 = myEngine.define("rpc2");
    auto rpc3 = myEngine.define("rpc3");

    tl::endpoint self_ep = myEngine.lookup(addr);

    int result1 = rpc1.on(self_ep)();
    int result2 = rpc2.on(self_ep)();
    int result3 = rpc3.on(self_ep)();

    REQUIRE(result1 == 1);
    REQUIRE(result2 == 2);
    REQUIRE(result3 == 3);
    myEngine.finalize();
}

} // TEST_SUITE
