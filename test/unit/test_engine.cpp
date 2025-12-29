/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for thallium::engine lifecycle and operations
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>

namespace tl = thallium;

TEST_SUITE("Engine") {

TEST_CASE("engine initialization client mode") {
    REQUIRE_NOTHROW({
        tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
        REQUIRE(myEngine.is_listening() == false);
        myEngine.finalize();
    });
}

TEST_CASE("engine initialization server mode") {
    REQUIRE_NOTHROW({
        tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
        REQUIRE(myEngine.is_listening() == true);
        myEngine.finalize();
    });
}

TEST_CASE("engine initialization with config") {
    const char* config = R"(
    {
        "argobots": {
            "pools": [
                {"name": "my_pool", "kind": "fifo_wait", "access": "mpmc"}
            ]
        }
    })";

    REQUIRE_NOTHROW({
        tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config, true);
        myEngine.finalize();
    });
}

TEST_CASE("engine self address") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    tl::endpoint self = myEngine.self();
    std::string addr_str = static_cast<std::string>(self);

    REQUIRE(!addr_str.empty());
    // Address may have protocol prefix like "na+sm://tcp://" so just check it contains tcp
    REQUIRE(addr_str.find("tcp") != std::string::npos);

    myEngine.finalize();
}

TEST_CASE("engine copy constructor") {
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);

    // Engine supports copy construction
    REQUIRE_NOTHROW({
        tl::engine copy(myEngine);
        REQUIRE(copy.is_listening() == myEngine.is_listening());
    });

    myEngine.finalize();
}

TEST_CASE("engine equality operators") {
    tl::engine myEngine1("tcp", THALLIUM_CLIENT_MODE);
    tl::engine myEngine2(myEngine1);  // Copy
    tl::engine myEngine3("tcp", THALLIUM_CLIENT_MODE);

    REQUIRE(myEngine1 == myEngine2);  // Same underlying Margo instance
    REQUIRE(myEngine1 != myEngine3);  // Different instances

    myEngine1.finalize();
    myEngine3.finalize();
}

TEST_CASE("engine finalize") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_NOTHROW(myEngine.finalize());

    // Note: is_listening() behavior after finalize may vary by implementation
    // The important part is that finalize() completes without error
}

TEST_CASE("engine wait for finalize") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::thread finalizer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        myEngine.finalize();
    });

    // wait_for_finalize should block until finalize is called
    REQUIRE_NOTHROW(myEngine.wait_for_finalize());

    finalizer.join();
}

TEST_CASE("engine is listening") {
    tl::engine server("tcp", THALLIUM_SERVER_MODE, true);
    tl::engine client("tcp", THALLIUM_CLIENT_MODE);

    REQUIRE(server.is_listening() == true);
    REQUIRE(client.is_listening() == false);

    server.finalize();
    client.finalize();
}

TEST_CASE("engine multiple instances") {
    // Multiple engines can coexist in the same process
    REQUIRE_NOTHROW({
        tl::engine engine1("tcp", THALLIUM_SERVER_MODE, true);
        tl::engine engine2("tcp", THALLIUM_SERVER_MODE, true);
        tl::engine engine3("tcp", THALLIUM_CLIENT_MODE);

        // Each should have different addresses
        std::string addr1 = static_cast<std::string>(engine1.self());
        std::string addr2 = static_cast<std::string>(engine2.self());

        REQUIRE(addr1 != addr2);

        engine1.finalize();
        engine2.finalize();
        engine3.finalize();
    });
}

TEST_CASE("engine get config") {
    const char* config = R"({"argobots": {"pools": []}})";
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config, true);

    // get_config should return configuration string
    REQUIRE_NOTHROW({
        std::string retrieved_config = myEngine.get_config();
        REQUIRE(!retrieved_config.empty());
    });

    myEngine.finalize();
}

TEST_CASE("engine enable remote shutdown") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_NOTHROW(myEngine.enable_remote_shutdown());

    myEngine.finalize();
}

TEST_CASE("engine lookup endpoint") {
    tl::engine server("tcp", THALLIUM_SERVER_MODE, true);
    tl::engine client("tcp", THALLIUM_CLIENT_MODE);

    std::string server_addr = static_cast<std::string>(server.self());

    // Client should be able to lookup server endpoint
    REQUIRE_NOTHROW({
        tl::endpoint ep = client.lookup(server_addr);
        std::string ep_addr = static_cast<std::string>(ep);
        REQUIRE(ep_addr == server_addr);
    });

    server.finalize();
    client.finalize();
}

TEST_CASE("engine reference counting") {
    // Test that Margo instance is properly reference counted
    tl::engine* engine1 = new tl::engine("tcp", THALLIUM_CLIENT_MODE);
    tl::engine engine2(*engine1);

    // Delete original, copy should still be valid
    delete engine1;

    REQUIRE_NOTHROW({
        bool listening = engine2.is_listening();
        (void)listening;
    });

    engine2.finalize();
}

TEST_CASE("engine finalize and wait") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Start a thread that will wait for finalization
    std::atomic<bool> wait_completed{false};
    std::thread waiter([&]() {
        myEngine.wait_for_finalize();
        wait_completed.store(true);
    });

    // Give waiter time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(wait_completed.load() == false);

    // Finalize should unblock the waiter
    myEngine.finalize();
    waiter.join();

    REQUIRE(wait_completed.load() == true);
}

TEST_CASE("engine client to server communication") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Define RPC on engine
    myEngine.define("test_rpc", [](const tl::request& req) {
        req.respond(42);
    });

    // Call RPC on itself
    auto rpc = myEngine.define("test_rpc");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int result = rpc.on(self_ep)();
    REQUIRE(result == 42);

    myEngine.finalize();
}

TEST_CASE("engine with progress thread") {
    // Test engine with use_progress_thread = true
    REQUIRE_NOTHROW({
        tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
        REQUIRE(myEngine.is_listening() == true);
        myEngine.finalize();
    });
}

} // TEST_SUITE
