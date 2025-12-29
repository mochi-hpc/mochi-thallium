/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for Argobots pool management
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>

namespace tl = thallium;

TEST_SUITE("Argobots Pools") {

TEST_CASE("get handler and progress pools") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Get the default handler pool
    tl::pool handler_pool = myEngine.get_handler_pool();
    REQUIRE(!handler_pool.is_null());
    REQUIRE(handler_pool);  // operator bool

    // Get the progress pool
    tl::pool progress_pool = myEngine.get_progress_pool();
    REQUIRE(!progress_pool.is_null());
    REQUIRE(progress_pool);

    myEngine.finalize();
}

TEST_CASE("create custom pool with managed wrapper") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool with MPMC access and fifo_wait kind
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    REQUIRE(!custom_pool->is_null());
    REQUIRE(custom_pool->get_access() == tl::pool::access::mpmc);

    myEngine.finalize();
}

TEST_CASE("create pools with different access types") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Test different access types
    auto pool_priv = tl::pool::create(tl::pool::access::priv);
    REQUIRE(pool_priv->get_access() == tl::pool::access::priv);

    auto pool_spsc = tl::pool::create(tl::pool::access::spsc);
    REQUIRE(pool_spsc->get_access() == tl::pool::access::spsc);

    auto pool_mpsc = tl::pool::create(tl::pool::access::mpsc);
    REQUIRE(pool_mpsc->get_access() == tl::pool::access::mpsc);

    auto pool_spmc = tl::pool::create(tl::pool::access::spmc);
    REQUIRE(pool_spmc->get_access() == tl::pool::access::spmc);

    auto pool_mpmc = tl::pool::create(tl::pool::access::mpmc);
    REQUIRE(pool_mpmc->get_access() == tl::pool::access::mpmc);

    myEngine.finalize();
}

TEST_CASE("create pools with different kinds") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // FIFO pool
    auto pool_fifo = tl::pool::create(tl::pool::access::mpmc, tl::pool::kind::fifo);
    REQUIRE(!pool_fifo->is_null());

    // FIFO wait pool
    auto pool_fifo_wait = tl::pool::create(tl::pool::access::mpmc, tl::pool::kind::fifo_wait);
    REQUIRE(!pool_fifo_wait->is_null());

    myEngine.finalize();
}

TEST_CASE("access pools by index via JSON config") {
    const char* config = R"(
    {
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "pool_1",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "pool_2",
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

    // Access pools by index
    auto pool0 = myEngine.pools()[0];
    auto pool1 = myEngine.pools()[1];
    auto pool2 = myEngine.pools()[2];

    REQUIRE(pool0.name() == "__primary__");
    REQUIRE(pool0.index() == 0);

    REQUIRE(pool1.name() == "pool_1");
    REQUIRE(pool1.index() == 1);

    REQUIRE(pool2.name() == "pool_2");
    REQUIRE(pool2.index() == 2);

    myEngine.finalize();
}

TEST_CASE("access pools by name via JSON config") {
    const char* config = R"(
    {
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "custom_pool",
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

    // Access pool by name
    auto custom_pool = myEngine.pools()["custom_pool"];
    REQUIRE(custom_pool.name() == "custom_pool");
    REQUIRE(custom_pool.index() == 1);

    myEngine.finalize();
}

TEST_CASE("list all pools") {
    const char* config = R"(
    {
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "pool_1",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "pool_2",
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

    // Get the number of pools
    std::size_t num_pools = myEngine.pools().size();
    REQUIRE(num_pools >= 3);  // At least the 3 we defined

    myEngine.finalize();
}

TEST_CASE("pool size queries") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    // Initially, pool should be empty or have minimal units
    std::size_t size = handler_pool.size();
    std::size_t total_size = handler_pool.total_size();

    // Both should be non-negative (size_t is unsigned)
    REQUIRE(size >= 0);
    REQUIRE(total_size >= 0);
    // total_size should be >= size
    REQUIRE(total_size >= size);

    myEngine.finalize();
}

TEST_CASE("pool id") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();
    tl::pool progress_pool = myEngine.get_progress_pool();

    // Each pool should have a unique id
    int handler_id = handler_pool.id();
    int progress_id = progress_pool.id();

    REQUIRE(handler_id >= 0);
    REQUIRE(progress_id >= 0);

    myEngine.finalize();
}

TEST_CASE("pool comparison operators") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();
    tl::pool handler_pool_copy = myEngine.get_handler_pool();
    tl::pool progress_pool = myEngine.get_progress_pool();

    // Same pool should be equal
    REQUIRE(handler_pool == handler_pool_copy);
    REQUIRE_FALSE(handler_pool != handler_pool_copy);

    // Different pools should not be equal
    REQUIRE(handler_pool != progress_pool);
    REQUIRE_FALSE(handler_pool == progress_pool);

    myEngine.finalize();
}

TEST_CASE("pool copy semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool original = myEngine.get_handler_pool();

    // Copy constructor
    tl::pool copy1(original);
    REQUIRE(copy1 == original);
    REQUIRE(!copy1.is_null());

    // Copy assignment
    tl::pool copy2;
    REQUIRE(copy2.is_null());
    copy2 = original;
    REQUIRE(copy2 == original);
    REQUIRE(!copy2.is_null());

    myEngine.finalize();
}

TEST_CASE("pool move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool original = myEngine.get_handler_pool();
    tl::pool original_copy = original;  // Keep a copy for comparison

    // Move constructor
    tl::pool moved1(std::move(original));
    REQUIRE(moved1 == original_copy);
    REQUIRE(!moved1.is_null());
    REQUIRE(original.is_null());  // Original should be null after move

    // Move assignment
    tl::pool moved2;
    REQUIRE(moved2.is_null());
    moved2 = std::move(moved1);
    REQUIRE(moved2 == original_copy);
    REQUIRE(!moved2.is_null());
    REQUIRE(moved1.is_null());  // moved1 should be null after move

    myEngine.finalize();
}

TEST_CASE("null pool checks") {
    // Default-constructed pool should be null
    tl::pool null_pool;
    REQUIRE(null_pool.is_null());
    REQUIRE_FALSE(null_pool);  // operator bool should return false

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool valid_pool = myEngine.get_handler_pool();
    REQUIRE_FALSE(valid_pool.is_null());
    REQUIRE(valid_pool);  // operator bool should return true

    myEngine.finalize();
}

TEST_CASE("RPC handlers with default handler pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Get the default handler pool
    tl::pool handler_pool = myEngine.get_handler_pool();

    // Define two RPCs on the same pool but with different provider IDs
    myEngine.define("rpc_a",
        [](const tl::request& req, int x) {
            req.respond(x + 10);
        },
        1, handler_pool);

    myEngine.define("rpc_b",
        [](const tl::request& req, int x) {
            req.respond(x + 20);
        },
        2, handler_pool);

    auto rpc_a = myEngine.define("rpc_a");
    auto rpc_b = myEngine.define("rpc_b");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Call both RPCs with different provider handles
    tl::provider_handle ph_a(self_ep, 1);
    tl::provider_handle ph_b(self_ep, 2);

    int result_a = rpc_a.on(ph_a)(5);
    int result_b = rpc_b.on(ph_b)(5);

    REQUIRE(result_a == 15);
    REQUIRE(result_b == 25);

    myEngine.finalize();
}

TEST_CASE("pool reference counting via pool object") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Get handler pool
    tl::pool handler_pool = myEngine.get_handler_pool();

    // Get initial reference count via pool object
    unsigned initial_count = myEngine.pools().ref_count(handler_pool);
    REQUIRE(initial_count >= 0);

    // Increment reference count
    myEngine.pools().ref_incr(handler_pool);
    unsigned after_incr = myEngine.pools().ref_count(handler_pool);
    REQUIRE(after_incr == initial_count + 1);

    // Release (decrement) reference count
    myEngine.pools().release(handler_pool);
    unsigned after_release = myEngine.pools().ref_count(handler_pool);
    REQUIRE(after_release == initial_count);

    myEngine.finalize();
}

TEST_CASE("pool access via native handle") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();
    tl::pool progress_pool = myEngine.get_progress_pool();

    // Access pools via their native handles
    ABT_pool handler_handle = handler_pool.native_handle();
    ABT_pool progress_handle = progress_pool.native_handle();

    REQUIRE(handler_handle != ABT_POOL_NULL);
    REQUIRE(progress_handle != ABT_POOL_NULL);
    REQUIRE(handler_handle != progress_handle);

    myEngine.finalize();
}

TEST_CASE("RPC association with custom pool via JSON config") {
    const char* config = R"(
    {
      "use_progress_thread": true,
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "rpc_pool",
            "kind": "fifo_wait",
            "access": "mpmc"
          }
        ],
        "xstreams": [
          {
            "name": "__primary__",
            "scheduler": {
              "type": "basic_wait",
              "pools": [0, 1]
            }
          }
        ]
      }
    }
    )";

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);
    std::string addr = static_cast<std::string>(myEngine.self());

    // Get the custom pool
    auto rpc_pool_proxy = myEngine.pools()["rpc_pool"];

    // Need to extract the actual pool from the proxy
    // The proxy object can be implicitly converted to pool
    tl::pool rpc_pool = myEngine.pools()[rpc_pool_proxy.index()];

    // Define RPC with custom pool (requires provider_id != 0)
    myEngine.define("pool_rpc",
        [](const tl::request& req, int x) {
            req.respond(x * 2);
        },
        1,  // provider_id must be non-zero for pool association
        rpc_pool);

    auto rpc = myEngine.define("pool_rpc");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Create provider handle for calling
    tl::provider_handle ph(self_ep, 1);

    int result = rpc.on(ph)(21);
    REQUIRE(result == 42);

    myEngine.finalize();
}

} // TEST_SUITE
