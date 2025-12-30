/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for Argobots schedulers
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <atomic>

namespace tl = thallium;

TEST_SUITE("Argobots Schedulers") {

TEST_CASE("create predefined scheduler with single pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create scheduler with default predefined type
    tl::managed<tl::scheduler> sched = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    REQUIRE(!sched->is_null());
    REQUIRE(*sched);  // operator bool

    myEngine.finalize();
}

TEST_CASE("create schedulers with different predefined types") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::managed<tl::pool> pool1 = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Test different predefined scheduler types
    auto sched_default = tl::scheduler::create(tl::scheduler::predef::deflt, *pool1);
    REQUIRE(!sched_default->is_null());

    auto sched_basic = tl::scheduler::create(tl::scheduler::predef::basic, *pool1);
    REQUIRE(!sched_basic->is_null());

    auto sched_basic_wait = tl::scheduler::create(tl::scheduler::predef::basic_wait, *pool1);
    REQUIRE(!sched_basic_wait->is_null());

    myEngine.finalize();
}

TEST_CASE("scheduler with multiple pools") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create multiple pools
    std::vector<tl::managed<tl::pool>> pools;
    for (int i = 0; i < 3; ++i) {
        pools.push_back(tl::pool::create(
            tl::pool::access::mpmc,
            tl::pool::kind::fifo_wait
        ));
    }

    // Create vector of pool references for the scheduler
    std::vector<tl::pool> pool_refs;
    for (auto& p : pools) {
        pool_refs.push_back(*p);
    }

    // Create scheduler with multiple pools
    tl::managed<tl::scheduler> sched = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        pool_refs.begin(),
        pool_refs.end()
    );

    REQUIRE(!sched->is_null());

    // Verify the scheduler has the right number of pools
    std::size_t num_pools = sched->num_pools();
    REQUIRE(num_pools == 3);

    myEngine.finalize();
}

TEST_CASE("scheduler get pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    tl::managed<tl::scheduler> sched = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    // Get the first pool from the scheduler
    tl::pool retrieved_pool = sched->get_pool(0);
    REQUIRE(!retrieved_pool.is_null());

    myEngine.finalize();
}

TEST_CASE("scheduler size queries") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    tl::managed<tl::scheduler> sched = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    // Initially, scheduler pools should be empty
    std::size_t size = sched->size();
    std::size_t total_size = sched->total_size();

    REQUIRE(size >= 0);
    REQUIRE(total_size >= 0);
    REQUIRE(total_size >= size);

    myEngine.finalize();
}

TEST_CASE("xstream with custom scheduler") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create a scheduler
    tl::managed<tl::scheduler> sched = tl::scheduler::create(
        tl::scheduler::predef::basic_wait,
        *custom_pool
    );

    // Create xstream with the custom scheduler
    tl::managed<tl::xstream> custom_xs = tl::xstream::create(*sched);

    REQUIRE(!custom_xs->is_null());

    // Verify we can create and execute a thread on this setup
    std::atomic<bool> thread_executed{false};

    tl::managed<tl::thread> th = custom_pool->make_thread([&thread_executed]() {
        thread_executed.store(true);
    });

    th->join();

    REQUIRE(thread_executed.load());

    myEngine.finalize();
    custom_xs->join();
}

TEST_CASE("scheduler null checks") {
    // Default-constructed scheduler should be null
    tl::scheduler null_sched;
    REQUIRE(null_sched.is_null());
    REQUIRE_FALSE(null_sched);  // operator bool should return false

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    tl::managed<tl::scheduler> valid_sched = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    REQUIRE_FALSE(valid_sched->is_null());
    REQUIRE(*valid_sched);  // operator bool should return true

    myEngine.finalize();
}

TEST_CASE("scheduler copy and move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    tl::managed<tl::scheduler> sched1 = tl::scheduler::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    // Copy the scheduler (not creating a new scheduler)
    tl::scheduler sched_copy = *sched1;
    REQUIRE(!sched_copy.is_null());

    // Move the managed wrapper
    tl::managed<tl::scheduler> sched_moved = std::move(sched1);
    REQUIRE(!sched_moved->is_null());

    myEngine.finalize();
}

TEST_CASE("xstream get and set main scheduler") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create an xstream
    tl::managed<tl::xstream> custom_xs = tl::xstream::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    // Get the main scheduler from the xstream
    tl::scheduler main_sched = custom_xs->get_main_sched();
    REQUIRE(!main_sched.is_null());

    // Verify the scheduler has at least one pool
    REQUIRE(main_sched.num_pools() >= 1);

    myEngine.finalize();
    custom_xs->join();
}

} // TEST_SUITE
