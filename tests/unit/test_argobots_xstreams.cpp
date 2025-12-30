/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for Argobots xstream (execution stream) management
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>

namespace tl = thallium;

TEST_SUITE("Argobots Xstreams") {

TEST_CASE("access xstreams by index via JSON config") {
    const char* config = R"(
    {
      "use_progress_thread": true,
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          }
        ],
        "xstreams": [
          {
            "name": "__primary__",
            "scheduler": {
              "type": "basic_wait",
              "pools": [0]
            }
          }
        ]
      }
    }
    )";

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);

    // Access xstream by index
    auto xs0 = myEngine.xstreams()[0];
    REQUIRE(xs0.name() == "__primary__");
    REQUIRE(xs0.index() == 0);

    myEngine.finalize();
}

TEST_CASE("access xstreams by name via JSON config") {
    const char* config = R"(
    {
      "use_progress_thread": true,
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          }
        ],
        "xstreams": [
          {
            "name": "__primary__",
            "scheduler": {
              "type": "basic_wait",
              "pools": [0]
            }
          }
        ]
      }
    }
    )";

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);

    // Access xstream by name
    auto xs_primary = myEngine.xstreams()["__primary__"];
    REQUIRE(xs_primary.name() == "__primary__");
    REQUIRE(xs_primary.index() == 0);

    myEngine.finalize();
}

TEST_CASE("list all xstreams") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Get the number of xstreams
    std::size_t num_xstreams = myEngine.xstreams().size();

    // Should have at least 1 xstream (primary)
    REQUIRE(num_xstreams >= 1);

    myEngine.finalize();
}

TEST_CASE("xstream self rank") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Get self rank (should be rank of primary ES, which is usually 0)
    int rank = tl::xstream::self_rank();
    REQUIRE(rank >= 0);

    myEngine.finalize();
}

TEST_CASE("xstream num") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Get number of running xstreams
    int num = tl::xstream::num();
    REQUIRE(num >= 1);

    myEngine.finalize();
}

TEST_CASE("xstream copy semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    auto xs_original = myEngine.xstreams()[0];

    // Copy constructor - copies the proxy, not creating a new xstream
    auto xs_copy1(xs_original);
    REQUIRE(xs_copy1.name() == xs_original.name());
    REQUIRE(xs_copy1.index() == xs_original.index());

    // Copy assignment
    auto xs_copy2 = xs_original;
    REQUIRE(xs_copy2.name() == xs_original.name());
    REQUIRE(xs_copy2.index() == xs_original.index());

    myEngine.finalize();
}

TEST_CASE("xstream reference counting via xstream object") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Access the primary xstream by index
    auto xs_proxy = myEngine.xstreams()[0];

    // Get initial reference count
    unsigned initial_count = myEngine.xstreams().ref_count(0);
    REQUIRE(initial_count >= 0);

    // Increment reference count
    myEngine.xstreams().ref_incr(0);
    unsigned after_incr = myEngine.xstreams().ref_count(0);
    REQUIRE(after_incr == initial_count + 1);

    // Release (decrement) reference count
    myEngine.xstreams().release(0);
    unsigned after_release = myEngine.xstreams().ref_count(0);
    REQUIRE(after_release == initial_count);

    myEngine.finalize();
}

TEST_CASE("create and manage custom xstream") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool for the xstream
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create a custom xstream with the pool
    tl::managed<tl::xstream> custom_xs = tl::xstream::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    REQUIRE(!custom_xs->is_null());
    REQUIRE(*custom_xs);  // operator bool

    // Join the xstream (wait for it to terminate)
    // Note: We need to finalize the engine first, which will cause xstreams to terminate
    myEngine.finalize();
    custom_xs->join();
}

TEST_CASE("xstream null checks") {
    // Default-constructed xstream should be null
    tl::xstream null_xs;
    REQUIRE(null_xs.is_null());
    REQUIRE_FALSE(null_xs);  // operator bool should return false

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Access primary xstream - should not be null
    auto xs_proxy = myEngine.xstreams()[0];
    // Note: The proxy itself isn't an xstream, so we can't directly check is_null()
    // But we can verify it has valid name and index
    REQUIRE(xs_proxy.index() == 0);

    myEngine.finalize();
}

TEST_CASE("xstream move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create a custom xstream
    tl::managed<tl::xstream> xs1 = tl::xstream::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    // Test that the xstream was created
    REQUIRE(!xs1->is_null());

    // Move the managed wrapper
    tl::managed<tl::xstream> xs2 = std::move(xs1);
    REQUIRE(!xs2->is_null());

    // Clean up
    myEngine.finalize();
    xs2->join();
}

TEST_CASE("multiple custom xstreams") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a shared pool for multiple xstreams
    tl::managed<tl::pool> shared_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create multiple xstreams
    std::vector<tl::managed<tl::xstream>> xstreams;
    for (int i = 0; i < 3; ++i) {
        xstreams.push_back(tl::xstream::create(
            tl::scheduler::predef::deflt,
            *shared_pool
        ));
    }

    // Verify all xstreams were created
    for (auto& xs : xstreams) {
        REQUIRE(!xs->is_null());
    }

    // Clean up
    myEngine.finalize();
    for (auto& xs : xstreams) {
        xs->join();
    }
}

} // TEST_SUITE
