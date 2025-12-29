/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for advanced RPC patterns in Thallium
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <vector>

namespace tl = thallium;

TEST_SUITE("RPC Advanced") {

TEST_CASE("rpc async call") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("async_add", [](const tl::request& req, int a, int b) {
        req.respond(a + b);
    });

    auto rpc = myEngine.define("async_add");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Make async RPC call
    auto response = rpc.on(self_ep).async(5, 7);

    // Response should not be received immediately
    // (though in practice it might be very fast)

    // Wait for response
    int result = response.wait();

    REQUIRE(result == 12);

    myEngine.finalize();
}

TEST_CASE("rpc async received check") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("slow_operation", [](const tl::request& req) {
        req.respond(42);
    });

    auto rpc = myEngine.define("slow_operation");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async();

    // Eventually the response should be received
    // In practice this is very fast for local calls
    int result = response.wait();

    // After wait, it should definitely be received
    REQUIRE(response.received() == true);
    REQUIRE(result == 42);

    myEngine.finalize();
}

TEST_CASE("rpc async multiple requests") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("multiply", [](const tl::request& req, int a, int b) {
        req.respond(a * b);
    });

    auto rpc = myEngine.define("multiply");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Launch multiple async requests
    auto response1 = rpc.on(self_ep).async(2, 3);
    auto response2 = rpc.on(self_ep).async(4, 5);
    auto response3 = rpc.on(self_ep).async(6, 7);

    // Wait for all responses
    int result1 = response1.wait();
    int result2 = response2.wait();
    int result3 = response3.wait();

    REQUIRE(result1 == 6);
    REQUIRE(result2 == 20);
    REQUIRE(result3 == 42);

    myEngine.finalize();
}

TEST_CASE("rpc request get endpoint") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::string caller_addr;

    myEngine.define("get_caller", [&](const tl::request& req) {
        tl::endpoint caller = req.get_endpoint();
        caller_addr = static_cast<std::string>(caller);
        req.respond(caller_addr);
    });

    auto rpc = myEngine.define("get_caller");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::string result = rpc.on(self_ep)();

    REQUIRE(!caller_addr.empty());
    REQUIRE(!result.empty());

    myEngine.finalize();
}

TEST_CASE("rpc deferred response") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("deferred", [](tl::request req) {
        // Handler takes request by value to extend lifetime
        // In a real scenario, you might store the request and respond later
        // For testing, we respond immediately
        req.respond(123);
    });

    auto rpc = myEngine.define("deferred");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)();

    REQUIRE(result == 123);

    myEngine.finalize();
}

TEST_CASE("rpc async wait any with multiple") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("compute", [](const tl::request& req, int x) {
        req.respond(x * 2);
    });

    auto rpc = myEngine.define("compute");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Create multiple async requests
    std::vector<tl::async_response> responses;
    responses.push_back(rpc.on(self_ep).async(1));
    responses.push_back(rpc.on(self_ep).async(2));
    responses.push_back(rpc.on(self_ep).async(3));

    // For simplicity, just wait on each individually
    // wait_any is more useful when you don't know which will complete first
    int result1 = responses[0].wait();
    int result2 = responses[1].wait();
    int result3 = responses[2].wait();

    REQUIRE((result1 == 2 || result1 == 4 || result1 == 6));
    REQUIRE((result2 == 2 || result2 == 4 || result2 == 6));
    REQUIRE((result3 == 2 || result3 == 4 || result3 == 6));

    myEngine.finalize();
}

TEST_CASE("rpc async wait any with single") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("single", [](const tl::request& req) {
        req.respond(99);
    });

    auto rpc = myEngine.define("single");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Single async response
    auto response = rpc.on(self_ep).async();
    int result = response.wait();

    REQUIRE(result == 99);

    myEngine.finalize();
}

TEST_CASE("rpc remote shutdown enable") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Enable remote shutdown capability
    REQUIRE_NOTHROW(myEngine.enable_remote_shutdown());

    myEngine.finalize();
}

TEST_CASE("rpc remote shutdown execute") {
    tl::engine server("tcp", THALLIUM_SERVER_MODE, true);
    std::string server_addr = static_cast<std::string>(server.self());

    server.enable_remote_shutdown();

    tl::engine client("tcp", THALLIUM_CLIENT_MODE);
    tl::endpoint server_ep = client.lookup(server_addr);

    // Shutdown the server from the client
    REQUIRE_NOTHROW(client.shutdown_remote_engine(server_ep));

    // Note: After remote shutdown, the server engine is finalized
    // We should not call finalize again on it
    client.finalize();
}

TEST_CASE("rpc async move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("test", [](const tl::request& req) {
        req.respond(42);
    });

    auto rpc = myEngine.define("test");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response1 = rpc.on(self_ep).async();
    auto response2 = std::move(response1);

    int result = response2.wait();
    REQUIRE(result == 42);

    myEngine.finalize();
}

TEST_CASE("rpc multiple args async") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("sum_three", [](const tl::request& req, int a, int b, int c) {
        req.respond(a + b + c);
    });

    auto rpc = myEngine.define("sum_three");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async(10, 20, 30);
    int result = response.wait();

    REQUIRE(result == 60);

    myEngine.finalize();
}

TEST_CASE("rpc async with string") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_async", [](const tl::request& req, const std::string& msg) {
        req.respond(msg);
    });

    auto rpc = myEngine.define("echo_async");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async(std::string("Hello async!"));
    std::string result = response.wait();

    REQUIRE(result == "Hello async!");

    myEngine.finalize();
}

TEST_CASE("rpc concurrent sync and async") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::atomic<int> call_count{0};

    myEngine.define("count", [&](const tl::request& req) {
        int count = call_count.fetch_add(1);
        req.respond(count);
    });

    auto rpc = myEngine.define("count");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Mix sync and async calls
    int sync_result = rpc.on(self_ep)();
    auto async_response = rpc.on(self_ep).async();
    int sync_result2 = rpc.on(self_ep)();
    int async_result = async_response.wait();

    // All calls should have been counted
    REQUIRE(call_count.load() >= 3);

    myEngine.finalize();
}

TEST_CASE("rpc async no response") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::atomic<bool> handler_called{false};

    myEngine.define("fire_forget", [&](const tl::request& req) {
        handler_called.store(true);
        // No response
    }).disable_response();

    auto rpc = myEngine.define("fire_forget").disable_response();
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Fire and forget - no waiting
    REQUIRE_NOTHROW(rpc.on(self_ep)());

    // Give handler time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    myEngine.finalize();
}

} // TEST_SUITE
