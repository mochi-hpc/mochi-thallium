/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for timed callbacks and timers
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/timer.hpp>
#include <atomic>
#include <chrono>

namespace tl = thallium;

TEST_SUITE("Timed Callbacks and Timers") {

// ============================================================================
// Timed Callback Tests
// ============================================================================

TEST_CASE("timed_callback basic execution") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<bool> callback_executed{false};

    {
        auto timed_cb = myEngine.create_timed_callback([&callback_executed]() {
            callback_executed.store(true);
        });

        // Start with 100ms timeout
        timed_cb.start(100);

        // Wait for callback to execute
        tl::thread::sleep(myEngine, 200);

        REQUIRE(callback_executed.load());
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback execution timing") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<bool> callback_executed{false};

    {
        auto timed_cb = myEngine.create_timed_callback([&callback_executed]() {
            callback_executed.store(true);
        });

        // Start with 200ms timeout
        timed_cb.start(200);

        // Check after 100ms - should not have executed yet
        tl::thread::sleep(myEngine, 100);
        REQUIRE_FALSE(callback_executed.load());

        // Wait another 150ms - should have executed by now
        tl::thread::sleep(myEngine, 150);
        REQUIRE(callback_executed.load());
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback cancel") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<bool> callback_executed{false};

    {
        auto timed_cb = myEngine.create_timed_callback([&callback_executed]() {
            callback_executed.store(true);
        });

        // Start with 500ms timeout
        timed_cb.start(500);

        // Wait 100ms
        tl::thread::sleep(myEngine, 100);

        // Cancel before execution
        timed_cb.cancel();

        // Wait long enough that it would have executed
        tl::thread::sleep(myEngine, 500);

        // Should not have executed
        REQUIRE_FALSE(callback_executed.load());
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback restart after execution") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<int> execution_count{0};

    {
        auto timed_cb = myEngine.create_timed_callback([&execution_count]() {
            execution_count.fetch_add(1);
        });

        // First execution
        timed_cb.start(100);
        tl::thread::sleep(myEngine, 200);
        REQUIRE(execution_count.load() == 1);

        // Second execution
        timed_cb.start(100);
        tl::thread::sleep(myEngine, 200);
        REQUIRE(execution_count.load() == 2);

        // Third execution
        timed_cb.start(100);
        tl::thread::sleep(myEngine, 200);
        REQUIRE(execution_count.load() == 3);
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback with captured state") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    int value = 10;
    std::atomic<int> result{0};

    {
        auto timed_cb = myEngine.create_timed_callback([&value, &result]() {
            result.store(value * 2);
        });

        timed_cb.start(100);
        tl::thread::sleep(myEngine, 200);

        REQUIRE(result.load() == 20);
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback move assignment") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<int> callback1_count{0};
    std::atomic<int> callback2_count{0};

    {
        auto timed_cb1 = myEngine.create_timed_callback([&callback1_count]() {
            callback1_count.fetch_add(1);
        });

        auto timed_cb2 = myEngine.create_timed_callback([&callback2_count]() {
            callback2_count.fetch_add(1);
        });

        // Move assign (note: move constructor has issues, so we only test move assignment)
        timed_cb2 = std::move(timed_cb1);

        // Start the moved-to callback
        timed_cb2.start(100);
        tl::thread::sleep(myEngine, 200);

        // callback1 should have executed (it was moved to timed_cb2)
        REQUIRE(callback1_count.load() == 1);
        // callback2 should not have executed (it was replaced)
        REQUIRE(callback2_count.load() == 0);
    }

    myEngine.finalize();
}

TEST_CASE("multiple timed_callbacks") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};
    std::atomic<int> counter3{0};

    {
        auto timed_cb1 = myEngine.create_timed_callback([&counter1]() {
            counter1.fetch_add(1);
        });

        auto timed_cb2 = myEngine.create_timed_callback([&counter2]() {
            counter2.fetch_add(1);
        });

        auto timed_cb3 = myEngine.create_timed_callback([&counter3]() {
            counter3.fetch_add(1);
        });

        // Start all three with different timeouts
        timed_cb1.start(100);
        timed_cb2.start(150);
        timed_cb3.start(200);

        // Wait for all to execute
        tl::thread::sleep(myEngine, 300);

        REQUIRE(counter1.load() == 1);
        REQUIRE(counter2.load() == 1);
        REQUIRE(counter3.load() == 1);
    }

    myEngine.finalize();
}

TEST_CASE("timed_callback zero timeout") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::atomic<bool> callback_executed{false};

    {
        auto timed_cb = myEngine.create_timed_callback([&callback_executed]() {
            callback_executed.store(true);
        });

        // Start with 0ms timeout (should execute immediately)
        timed_cb.start(0);

        // Small sleep to allow execution
        tl::thread::sleep(myEngine, 50);

        REQUIRE(callback_executed.load());
    }

    myEngine.finalize();
}

// ============================================================================
// Timer Tests
// ============================================================================

TEST_CASE("timer basic start stop read") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t;

    t.start();
    tl::thread::sleep(myEngine, 100);
    t.stop();

    double elapsed = t.read();

    // Should have elapsed at least 100ms (0.1 seconds)
    // Allow some tolerance for timing variations
    REQUIRE(elapsed >= 0.08);
    REQUIRE(elapsed <= 0.2);

    myEngine.finalize();
}

TEST_CASE("timer multiple measurements") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t;

    // First measurement
    t.start();
    tl::thread::sleep(myEngine, 50);
    t.stop();
    double elapsed1 = t.read();

    // Second measurement
    t.start();
    tl::thread::sleep(myEngine, 100);
    t.stop();
    double elapsed2 = t.read();

    // Second should be longer
    REQUIRE(elapsed2 > elapsed1);

    myEngine.finalize();
}

TEST_CASE("timer wtime") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    double t1 = tl::timer::wtime();
    tl::thread::sleep(myEngine, 50);
    double t2 = tl::timer::wtime();

    double elapsed = t2 - t1;

    // Should be at least 50ms
    REQUIRE(elapsed >= 0.04);
    REQUIRE(elapsed <= 0.15);

    myEngine.finalize();
}

TEST_CASE("timer overhead") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    double oh = tl::timer::overhead();

    // Overhead should be positive but very small (< 1ms)
    REQUIRE(oh >= 0.0);
    REQUIRE(oh < 0.001);

    myEngine.finalize();
}

TEST_CASE("timer copy semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t1;
    t1.start();
    tl::thread::sleep(myEngine, 50);
    t1.stop();

    // Copy constructor
    tl::timer t2(t1);
    double elapsed1 = t1.read();
    double elapsed2 = t2.read();

    // Both should have same elapsed time
    REQUIRE(elapsed1 == elapsed2);

    // Copy assignment
    tl::timer t3;
    t3 = t1;
    double elapsed3 = t3.read();
    REQUIRE(elapsed3 == elapsed1);

    myEngine.finalize();
}

TEST_CASE("timer move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t1;
    t1.start();
    tl::thread::sleep(myEngine, 50);
    t1.stop();
    double elapsed1 = t1.read();

    // Move construct
    tl::timer t2(std::move(t1));
    double elapsed2 = t2.read();
    REQUIRE(elapsed2 == elapsed1);

    // Move assign
    tl::timer t3;
    t3 = std::move(t2);
    double elapsed3 = t3.read();
    REQUIRE(elapsed3 == elapsed1);

    myEngine.finalize();
}

TEST_CASE("timer explicit conversion to double") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t;
    t.start();
    tl::thread::sleep(myEngine, 50);
    t.stop();

    // Use explicit conversion operator
    double elapsed = static_cast<double>(t);

    REQUIRE(elapsed >= 0.04);
    REQUIRE(elapsed <= 0.15);

    myEngine.finalize();
}

TEST_CASE("timer native handle") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::timer t;

    ABT_timer native = t.native_handle();
    REQUIRE(native != ABT_TIMER_NULL);

    myEngine.finalize();
}

} // TEST_SUITE
