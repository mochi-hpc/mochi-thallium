/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for finalization callbacks
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <vector>
#include <string>

namespace tl = thallium;

TEST_SUITE("Finalization") {

TEST_CASE("push and execute finalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    bool callback_executed = false;

    myEngine.push_finalize_callback([&callback_executed]() {
        callback_executed = true;
    });

    REQUIRE(callback_executed == false);

    myEngine.finalize();

    REQUIRE(callback_executed == true);
}

TEST_CASE("multiple finalize callbacks LIFO order") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<int> execution_order;

    myEngine.push_finalize_callback([&execution_order]() {
        execution_order.push_back(1);
    });

    myEngine.push_finalize_callback([&execution_order]() {
        execution_order.push_back(2);
    });

    myEngine.push_finalize_callback([&execution_order]() {
        execution_order.push_back(3);
    });

    myEngine.finalize();

    // Callbacks should execute in reverse order (LIFO)
    REQUIRE(execution_order.size() == 3);
    REQUIRE(execution_order[0] == 3);
    REQUIRE(execution_order[1] == 2);
    REQUIRE(execution_order[2] == 1);
}

TEST_CASE("pop finalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    bool callback1_executed = false;
    bool callback2_executed = false;

    myEngine.push_finalize_callback([&callback1_executed]() {
        callback1_executed = true;
    });

    myEngine.push_finalize_callback([&callback2_executed]() {
        callback2_executed = true;
    });

    // Pop the second callback
    auto popped = myEngine.pop_finalize_callback();
    REQUIRE(popped != nullptr);

    myEngine.finalize();

    // Only first callback should execute
    REQUIRE(callback1_executed == true);
    REQUIRE(callback2_executed == false);
}

TEST_CASE("top finalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    int value = 0;

    myEngine.push_finalize_callback([&value]() {
        value = 10;
    });

    myEngine.push_finalize_callback([&value]() {
        value = 20;
    });

    // Get top callback without removing it
    auto top = myEngine.top_finalize_callback();
    REQUIRE(top != nullptr);

    // Execute it manually
    top();
    REQUIRE(value == 20);

    // Reset and finalize - both should still execute
    value = 0;
    myEngine.finalize();

    // Last one to execute is the first one pushed
    REQUIRE(value == 10);
}

TEST_CASE("finalize callbacks with owner") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    int owner1_id = 1;
    int owner2_id = 2;
    void* owner1 = &owner1_id;
    void* owner2 = &owner2_id;

    bool callback1_executed = false;
    bool callback2_executed = false;

    myEngine.push_finalize_callback(owner1, [&callback1_executed]() {
        callback1_executed = true;
    });

    myEngine.push_finalize_callback(owner2, [&callback2_executed]() {
        callback2_executed = true;
    });

    // Pop callback for owner1
    auto popped = myEngine.pop_finalize_callback(owner1);
    REQUIRE(popped != nullptr);

    myEngine.finalize();

    // Only owner2's callback should execute
    REQUIRE(callback1_executed == false);
    REQUIRE(callback2_executed == true);
}

TEST_CASE("prefinalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    bool prefinalize_executed = false;
    bool finalize_executed = false;

    myEngine.push_prefinalize_callback([&prefinalize_executed]() {
        prefinalize_executed = true;
    });

    myEngine.push_finalize_callback([&finalize_executed]() {
        finalize_executed = true;
    });

    myEngine.finalize();

    // Both should have executed
    REQUIRE(prefinalize_executed == true);
    REQUIRE(finalize_executed == true);
}

TEST_CASE("prefinalize executes before finalize") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<std::string> execution_order;

    myEngine.push_prefinalize_callback([&execution_order]() {
        execution_order.push_back("prefinalize");
    });

    myEngine.push_finalize_callback([&execution_order]() {
        execution_order.push_back("finalize");
    });

    myEngine.finalize();

    REQUIRE(execution_order.size() == 2);
    REQUIRE(execution_order[0] == "prefinalize");
    REQUIRE(execution_order[1] == "finalize");
}

TEST_CASE("pop prefinalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    bool callback1_executed = false;
    bool callback2_executed = false;

    myEngine.push_prefinalize_callback([&callback1_executed]() {
        callback1_executed = true;
    });

    myEngine.push_prefinalize_callback([&callback2_executed]() {
        callback2_executed = true;
    });

    // Pop the second callback
    auto popped = myEngine.pop_prefinalize_callback();
    REQUIRE(popped != nullptr);

    myEngine.finalize();

    // Only first callback should execute
    REQUIRE(callback1_executed == true);
    REQUIRE(callback2_executed == false);
}

TEST_CASE("multiple prefinalize callbacks LIFO order") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<int> execution_order;

    myEngine.push_prefinalize_callback([&execution_order]() {
        execution_order.push_back(1);
    });

    myEngine.push_prefinalize_callback([&execution_order]() {
        execution_order.push_back(2);
    });

    myEngine.push_prefinalize_callback([&execution_order]() {
        execution_order.push_back(3);
    });

    myEngine.finalize();

    // Callbacks should execute in reverse order (LIFO)
    REQUIRE(execution_order.size() == 3);
    REQUIRE(execution_order[0] == 3);
    REQUIRE(execution_order[1] == 2);
    REQUIRE(execution_order[2] == 1);
}

TEST_CASE("resource cleanup in finalize callback") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Simulate resource that needs cleanup
    int* resource = new int(42);
    bool resource_cleaned = false;

    myEngine.push_finalize_callback([resource, &resource_cleaned]() {
        delete resource;
        resource_cleaned = true;
    });

    myEngine.finalize();

    REQUIRE(resource_cleaned == true);
}

} // TEST_SUITE
