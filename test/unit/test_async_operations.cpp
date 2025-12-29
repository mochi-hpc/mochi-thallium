/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for advanced async operation patterns
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <vector>

namespace tl = thallium;

// Custom type for async operations
struct AsyncData {
    int id;
    std::string payload;

    AsyncData() : id(0) {}
    AsyncData(int i, const std::string& p) : id(i), payload(p) {}

    template<typename A>
    void serialize(A& ar) {
        ar & id;
        ar & payload;
    }

    bool operator==(const AsyncData& other) const {
        return id == other.id && payload == other.payload;
    }
};

TEST_SUITE("Async Operations") {

TEST_CASE("async response lifecycle") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("compute", [](const tl::request& req, int x) {
        req.respond(x * 2);
    });

    auto rpc = myEngine.define("compute");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async(21);

    // Before wait, received() may or may not be true (race condition)
    // After wait, it should definitely be true
    int result = response.wait();
    REQUIRE(response.received() == true);
    REQUIRE(result == 42);

    myEngine.finalize();
}

TEST_CASE("async with custom type") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("process_async_data", [](const tl::request& req, const AsyncData& data) {
        AsyncData result(data.id * 2, data.payload + "_processed");
        req.respond(result);
    });

    auto rpc = myEngine.define("process_async_data");
    tl::endpoint self_ep = myEngine.lookup(addr);

    AsyncData input(10, "test");
    auto response = rpc.on(self_ep).async(input);

    AsyncData result = response.wait();

    REQUIRE(result.id == 20);
    REQUIRE(result.payload == "test_processed");

    myEngine.finalize();
}

TEST_CASE("multiple async operations in sequence") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("increment", [](const tl::request& req, int x) {
        req.respond(x + 1);
    });

    auto rpc = myEngine.define("increment");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Launch multiple async operations in sequence
    auto resp1 = rpc.on(self_ep).async(1);
    int result1 = resp1.wait();
    REQUIRE(result1 == 2);

    auto resp2 = rpc.on(self_ep).async(result1);
    int result2 = resp2.wait();
    REQUIRE(result2 == 3);

    auto resp3 = rpc.on(self_ep).async(result2);
    int result3 = resp3.wait();
    REQUIRE(result3 == 4);

    myEngine.finalize();
}

TEST_CASE("async with vector return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("generate_sequence", [](const tl::request& req, int n) {
        std::vector<int> result;
        for (int i = 0; i < n; ++i) {
            result.push_back(i);
        }
        req.respond(result);
    });

    auto rpc = myEngine.define("generate_sequence");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async(5);
    std::vector<int> result = response.wait();

    REQUIRE(result.size() == 5);
    for (int i = 0; i < 5; ++i) {
        REQUIRE(result[i] == i);
    }

    myEngine.finalize();
}

TEST_CASE("many concurrent async operations") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("square", [](const tl::request& req, int x) {
        req.respond(x * x);
    });

    auto rpc = myEngine.define("square");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Launch many concurrent async operations
    const int num_ops = 20;
    std::vector<tl::async_response> responses;

    for (int i = 0; i < num_ops; ++i) {
        responses.push_back(rpc.on(self_ep).async(i));
    }

    // Wait for all and verify results
    for (int i = 0; i < num_ops; ++i) {
        int result = responses[i].wait();
        REQUIRE(result == i * i);
    }

    myEngine.finalize();
}

TEST_CASE("async operation with no arguments") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    int counter = 0;
    myEngine.define("get_next", [&counter](const tl::request& req) {
        req.respond(++counter);
    });

    auto rpc = myEngine.define("get_next");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto resp1 = rpc.on(self_ep).async();
    auto resp2 = rpc.on(self_ep).async();
    auto resp3 = rpc.on(self_ep).async();

    int result1 = resp1.wait();
    int result2 = resp2.wait();
    int result3 = resp3.wait();

    // Results should be sequential
    REQUIRE(result1 >= 1);
    REQUIRE(result2 >= 1);
    REQUIRE(result3 >= 1);
    REQUIRE(result1 + result2 + result3 == 6);  // 1 + 2 + 3

    myEngine.finalize();
}

TEST_CASE("async with multiple return values") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("divide", [](const tl::request& req, int a, int b) {
        int quotient = a / b;
        int remainder = a % b;
        req.respond(quotient, remainder);
    });

    auto rpc = myEngine.define("divide");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto response = rpc.on(self_ep).async(17, 5);

    // Wait and unpack multiple return values
    auto result = response.wait();
    int quotient = std::get<0>(result.as<int, int>());
    int remainder = std::get<1>(result.as<int, int>());

    REQUIRE(quotient == 3);
    REQUIRE(remainder == 2);

    myEngine.finalize();
}

} // TEST_SUITE
