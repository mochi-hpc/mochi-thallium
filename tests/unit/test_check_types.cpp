/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for the THALLIUM_DEBUG_RPC_TYPES type-checking feature.
 *
 * This file must be compiled with -DTHALLIUM_DEBUG_RPC_TYPES (by linking
 * against the thallium_check_types CMake target) so that type names are
 * serialized alongside RPC data and compared on the receiving end.
 *
 * Root cause of the current bug
 * ─────────────────────────────
 * Both the arg-encoding path (callable_remote_procedure.hpp) and the
 * respond() path (request.hpp) pass arguments through std::cref() before
 * building the tuple, e.g.:
 *
 *   std::make_tuple(std::cref(args)...)
 *
 * std::make_tuple unwraps std::reference_wrapper<const T> into "const T&",
 * so the resulting tuple element type is "const T&".  The type name sent on
 * the wire therefore includes the const-reference qualification ("int const&"
 * instead of "int").  The server-side decode uses std::decay<T>::type ("int"),
 * so the comparison always fails for any non-void type.
 *
 * Tests 2–5 document this bug; they will fail until the fix strips
 * cv-qualifiers and references from both sides before comparing.
 *
 * Tests 2–5 use rpc.on(ep).timed(..., args) instead of rpc.on(ep)(args) so
 * that a missing response (server rejected the decode) causes a tl::timeout
 * throw rather than an indefinite hang or a process abort.  This way every
 * test case runs independently even before the fix is applied.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

// Short deadline used by currently-broken tests so each one fails quickly.
static constexpr auto kCallTimeout = std::chrono::seconds(2);

TEST_SUITE("RPC Type Checking") {

// ── Test 1 ──────────────────────────────────────────────────────────────────
// Void args, void return: proc_void_object hard-codes "void" on both sides,
// so this case is unaffected by the const-ref bug and must work right now.
TEST_CASE("type check void args and void return") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    bool handler_called = false;
    myEngine.define("void_void_check", [&](const tl::request& req) {
        handler_called = true;
        req.respond();
    });

    auto rpc = myEngine.define("void_void_check");
    tl::endpoint ep = myEngine.lookup(addr);

    REQUIRE_NOTHROW(rpc.on(ep)());
    REQUIRE(handler_called == true);

    myEngine.finalize();
}

// ── Test 2 ──────────────────────────────────────────────────────────────────
// Matching int arg, void return.
// BUG: client encodes "int const&"; server expects "int" → mismatch →
// server never responds → tl::timeout after kCallTimeout.
// After the fix (qualifiers stripped): should pass cleanly.
TEST_CASE("type check matching int arg succeeds") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    int received = -1;
    myEngine.define("int_arg_check", [&](const tl::request& req, int x) {
        received = x;
        req.respond();
    });

    auto rpc = myEngine.define("int_arg_check");
    tl::endpoint ep = myEngine.lookup(addr);

    REQUIRE_NOTHROW(rpc.on(ep).timed(kCallTimeout, 42));
    REQUIRE(received == 42);

    myEngine.finalize();
}

// ── Test 3 ──────────────────────────────────────────────────────────────────
// Matching std::string arg, void return.  Same const-ref bug as test 2.
TEST_CASE("type check matching string arg succeeds") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    std::string received;
    myEngine.define("str_arg_check", [&](const tl::request& req, std::string s) {
        received = s;
        req.respond();
    });

    auto rpc = myEngine.define("str_arg_check");
    tl::endpoint ep = myEngine.lookup(addr);

    REQUIRE_NOTHROW(rpc.on(ep).timed(kCallTimeout, std::string("hello")));
    REQUIRE(received == "hello");

    myEngine.finalize();
}

// ── Test 4 ──────────────────────────────────────────────────────────────────
// No arg, int return.
// BUG: req.respond(42) also uses std::cref, encoding "int const&"; the
// client decodes as "int" → mismatch → packed_data::as<int>() throws.
// After the fix: should pass cleanly.
TEST_CASE("type check matching return value succeeds") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("int_return_check", [](const tl::request& req) {
        req.respond(42);
    });

    auto rpc = myEngine.define("int_return_check");
    tl::endpoint ep = myEngine.lookup(addr);

    // The server responds, so no hang risk; the throw comes from client-side
    // packed_data decode due to "int const&" vs "int" mismatch.
    int result = -1;
    REQUIRE_NOTHROW(result = (int)rpc.on(ep)());
    REQUIRE(result == 42);

    myEngine.finalize();
}

// ── Test 5 ──────────────────────────────────────────────────────────────────
// Matching int arg and int return (both checked together).
TEST_CASE("type check matching int arg and return value succeeds") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("int_int_check", [](const tl::request& req, int x) {
        req.respond(x * 2);
    });

    auto rpc = myEngine.define("int_int_check");
    tl::endpoint ep = myEngine.lookup(addr);

    int result = -1;
    REQUIRE_NOTHROW(result = (int)rpc.on(ep).timed(kCallTimeout, 21));
    REQUIRE(result == 42);

    myEngine.finalize();
}

// ── Test 6 ──────────────────────────────────────────────────────────────────
// Intentional arg type mismatch: server expects int, client sends double.
// The type checker must detect the mismatch and not invoke the handler.
// We use a 1-second RPC timeout so the call doesn't block forever if the
// server sends no response after detecting the mismatch.
TEST_CASE("type check rejects mismatched arg type") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    bool handler_called = false;
    myEngine.define("int_mismatch_check", [&](const tl::request& req, int x) {
        handler_called = true;
        req.respond();
    });

    auto rpc = myEngine.define("int_mismatch_check");
    tl::endpoint ep = myEngine.lookup(addr);

    // double ≠ int → type error on server; timed() prevents indefinite hang.
    REQUIRE_THROWS(rpc.on(ep).timed(std::chrono::seconds(1), 3.14));
    REQUIRE_FALSE(handler_called);

    myEngine.finalize();
}

// ── Test 7 ──────────────────────────────────────────────────────────────────
// Intentional return type mismatch: server responds with int, client decodes
// as std::string.  The server does reply (handler runs), so we don't need a
// separate timeout; the throw comes from client-side decode.
TEST_CASE("type check rejects mismatched return type") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("int_return_mismatch_check", [](const tl::request& req) {
        req.respond(42);  // sends int
    });

    auto rpc = myEngine.define("int_return_mismatch_check");
    tl::endpoint ep = myEngine.lookup(addr);

    // Server sends int, client tries to decode as string → type mismatch → throws.
    REQUIRE_THROWS({
        std::string s = rpc.on(ep)();
    });

    myEngine.finalize();
}

} // TEST_SUITE("RPC Type Checking")
