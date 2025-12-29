/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for thallium::provider system
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

// Example provider class
class test_provider : public tl::provider<test_provider> {
public:
    test_provider(tl::engine& e, uint16_t provider_id)
        : tl::provider<test_provider>(e, provider_id) {

        define("add", &test_provider::add);
        define("get_id", &test_provider::get_id);
    }

    void add(const tl::request& req, int a, int b) {
        req.respond(a + b);
    }

    void get_id(const tl::request& req) {
        req.respond(get_provider_id());
    }
};

TEST_SUITE("Provider") {

TEST_CASE("provider basic creation") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_NOTHROW({
        test_provider provider(myEngine, 1);
    });

    myEngine.finalize();
}

TEST_CASE("provider with identity") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    test_provider provider(myEngine, 1);

    // Provider should have the assigned ID
    REQUIRE(provider.get_provider_id() == 1);

    myEngine.finalize();
}

TEST_CASE("provider id uniqueness") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    test_provider provider1(myEngine, 1);
    test_provider provider2(myEngine, 2);

    REQUIRE(provider1.get_provider_id() != provider2.get_provider_id());
    REQUIRE(provider1.get_provider_id() == 1);
    REQUIRE(provider2.get_provider_id() == 2);

    myEngine.finalize();
}

TEST_CASE("provider rpc registration") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Provider registers RPCs in constructor
    test_provider provider(myEngine, 1);

    // RPCs should be registered and callable
    REQUIRE_NOTHROW({
        std::string addr = static_cast<std::string>(myEngine.self());
        tl::endpoint self_ep = myEngine.lookup(addr);
        tl::provider_handle ph(self_ep, 1);

        auto rpc = myEngine.define("add");
        int result = rpc.on(ph)(5, 7);
        REQUIRE(result == 12);
    });

    myEngine.finalize();
}

TEST_CASE("provider specific calling") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider(myEngine, 42);

    tl::endpoint self_ep = myEngine.lookup(addr);
    tl::provider_handle ph(self_ep, 42);

    auto rpc = myEngine.define("get_id");
    uint16_t provider_id = rpc.on(ph)();

    REQUIRE(provider_id == 42);

    myEngine.finalize();
}

TEST_CASE("provider member function rpc") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider(myEngine, 1);

    tl::endpoint self_ep = myEngine.lookup(addr);
    tl::provider_handle ph(self_ep, 1);

    // Call member function RPC
    auto rpc = myEngine.define("add");
    int result = rpc.on(ph)(10, 20);

    REQUIRE(result == 30);

    myEngine.finalize();
}

TEST_CASE("provider handle creation") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider(myEngine, 5);

    tl::endpoint self_ep = myEngine.lookup(addr);

    REQUIRE_NOTHROW({
        tl::provider_handle ph(self_ep, 5);
    });

    myEngine.finalize();
}

TEST_CASE("provider handle targeting") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider1(myEngine, 1);
    test_provider provider2(myEngine, 2);

    tl::endpoint self_ep = myEngine.lookup(addr);
    tl::provider_handle ph1(self_ep, 1);
    tl::provider_handle ph2(self_ep, 2);

    auto rpc = myEngine.define("get_id");

    uint16_t id1 = rpc.on(ph1)();
    uint16_t id2 = rpc.on(ph2)();

    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);

    myEngine.finalize();
}

TEST_CASE("provider multiple per engine") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Create multiple providers on same engine
    test_provider provider1(myEngine, 1);
    test_provider provider2(myEngine, 2);
    test_provider provider3(myEngine, 3);

    tl::endpoint self_ep = myEngine.lookup(addr);

    // Each provider should be callable independently
    auto rpc = myEngine.define("add");

    int result1 = rpc.on(tl::provider_handle(self_ep, 1))(1, 1);
    int result2 = rpc.on(tl::provider_handle(self_ep, 2))(2, 2);
    int result3 = rpc.on(tl::provider_handle(self_ep, 3))(3, 3);

    REQUIRE(result1 == 2);
    REQUIRE(result2 == 4);
    REQUIRE(result3 == 6);

    myEngine.finalize();
}

TEST_CASE("provider get engine") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    test_provider provider(myEngine, 1);

    // Provider should have access to its engine (returns a copy)
    tl::engine engine_copy = provider.get_engine();

    // Verify the engine is valid by using it
    std::string addr = static_cast<std::string>(engine_copy.self());
    REQUIRE(!addr.empty());

    myEngine.finalize();
}

TEST_CASE("provider same rpc different providers") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider1(myEngine, 10);
    test_provider provider2(myEngine, 20);

    tl::endpoint self_ep = myEngine.lookup(addr);

    // Same RPC name on different providers
    auto rpc = myEngine.define("add");

    int result1 = rpc.on(tl::provider_handle(self_ep, 10))(5, 5);
    int result2 = rpc.on(tl::provider_handle(self_ep, 20))(7, 3);

    REQUIRE(result1 == 10);
    REQUIRE(result2 == 10);

    myEngine.finalize();
}

TEST_CASE("provider zero id") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Provider ID 0 should work
    REQUIRE_NOTHROW({
        test_provider provider(myEngine, 0);
        REQUIRE(provider.get_provider_id() == 0);
    });

    myEngine.finalize();
}

TEST_CASE("provider handle comparison") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    test_provider provider(myEngine, 1);

    tl::endpoint self_ep = myEngine.lookup(addr);
    tl::provider_handle ph1(self_ep, 1);
    tl::provider_handle ph2(self_ep, 1);
    tl::provider_handle ph3(self_ep, 2);

    // Note: provider_handle inherits from endpoint and uses endpoint's
    // comparison operators, which only compare addresses, not provider IDs
    REQUIRE(ph1 == ph2);
    REQUIRE(ph1 == ph3);  // Same endpoint, different provider ID, but still equal

    // Provider IDs are accessible separately
    REQUIRE(ph1.provider_id() == 1);
    REQUIRE(ph3.provider_id() == 2);

    myEngine.finalize();
}

} // TEST_SUITE
