/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for thallium::endpoint operations
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>

namespace tl = thallium;

TEST_SUITE("Endpoint") {

TEST_CASE("endpoint self") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::endpoint self = myEngine.self();
    std::string addr = static_cast<std::string>(self);

    REQUIRE(!addr.empty());
    REQUIRE(addr.find("tcp") != std::string::npos);

    myEngine.finalize();
}

TEST_CASE("endpoint lookup") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Should be able to lookup own address
    tl::endpoint ep = myEngine.lookup(addr);
    std::string ep_addr = static_cast<std::string>(ep);

    REQUIRE(ep_addr == addr);

    myEngine.finalize();
}

TEST_CASE("endpoint comparison") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    tl::endpoint ep1 = myEngine.self();
    tl::endpoint ep2 = myEngine.lookup(addr);

    // Same address should compare equal
    REQUIRE(ep1 == ep2);

    myEngine.finalize();
}

TEST_CASE("endpoint inequality") {
    tl::engine engine1("tcp", THALLIUM_SERVER_MODE, true);
    tl::engine engine2("tcp", THALLIUM_SERVER_MODE, true);

    tl::endpoint ep1 = engine1.self();
    tl::endpoint ep2 = engine2.self();

    // Different engines should have different endpoints
    REQUIRE(ep1 != ep2);

    engine1.finalize();
    engine2.finalize();
}

TEST_CASE("endpoint copy semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::endpoint original = myEngine.self();
    tl::endpoint copy = original;

    // Copy should equal original
    REQUIRE(copy == original);

    // Both should have same string representation
    std::string orig_addr = static_cast<std::string>(original);
    std::string copy_addr = static_cast<std::string>(copy);
    REQUIRE(orig_addr == copy_addr);

    myEngine.finalize();
}

TEST_CASE("endpoint move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    tl::endpoint original = myEngine.self();
    tl::endpoint moved = std::move(original);

    // Moved endpoint should have the address
    std::string moved_addr = static_cast<std::string>(moved);
    REQUIRE(moved_addr == addr);

    myEngine.finalize();
}

TEST_CASE("endpoint to string") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::endpoint ep = myEngine.self();
    std::string addr = static_cast<std::string>(ep);

    // Should be able to convert to string
    REQUIRE(!addr.empty());
    REQUIRE(addr.find("tcp") != std::string::npos);

    // Should contain port number (format: protocol://host:port)
    REQUIRE(addr.find(":") != std::string::npos);

    myEngine.finalize();
}

TEST_CASE("endpoint is null") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::endpoint valid_ep = myEngine.self();

    // Valid endpoint should not be null
    REQUIRE(valid_ep.is_null() == false);

    myEngine.finalize();
}

TEST_CASE("endpoint invalid address") {
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);

    // Looking up invalid addresses should throw
    REQUIRE_THROWS({
        tl::endpoint ep = myEngine.lookup("invalid://address:1234");
    });

    REQUIRE_THROWS({
        tl::endpoint ep = myEngine.lookup("not-an-address");
    });

    REQUIRE_THROWS({
        tl::endpoint ep = myEngine.lookup("");
    });

    myEngine.finalize();
}

TEST_CASE("endpoint null address to_string") {
    // Test that converting null endpoint to string returns empty string
    // Covers endpoint.hpp line 145
    tl::endpoint null_ep;  // Default constructed, null address

    std::string str = static_cast<std::string>(null_ep);  // Line 145
    REQUIRE(str.empty());
}

TEST_CASE("endpoint stream operator") {
    // Test streaming endpoint to output stream
    // Covers endpoint.hpp lines 233-235
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    tl::endpoint ep = myEngine.self();

    std::stringstream ss;
    ss << ep;  // Lines 233-235

    std::string streamed = ss.str();
    std::string direct = static_cast<std::string>(ep);

    REQUIRE(streamed == direct);
    REQUIRE(!streamed.empty());

    myEngine.finalize();
}

TEST_CASE("endpoint copy constructor from non-null") {
    // Test copy constructor assertion path
    // Covers endpoint.hpp line 77
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    tl::endpoint ep1 = myEngine.self();

    tl::endpoint ep2(ep1);  // Copy constructor - Line 77

    REQUIRE(ep1 == ep2);
    REQUIRE(!ep2.is_null());

    std::string addr1 = static_cast<std::string>(ep1);
    std::string addr2 = static_cast<std::string>(ep2);
    REQUIRE(addr1 == addr2);

    myEngine.finalize();
}

} // TEST_SUITE
