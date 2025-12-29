/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for engine configuration
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>

namespace tl = thallium;

TEST_SUITE("Configuration") {

TEST_CASE("basic engine without config") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Should be able to get config even without explicit JSON
    std::string config = myEngine.get_config();
    REQUIRE(!config.empty());

    myEngine.finalize();
}

TEST_CASE("engine with JSON config") {
    const char* config = R"(
    {
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

    REQUIRE_NOTHROW({
        // Note: Don't use progress thread (true) when providing JSON config
        tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);
        myEngine.finalize();
    });
}

TEST_CASE("get config returns valid JSON") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::string config = myEngine.get_config();

    // Config should be non-empty and contain expected JSON structure
    REQUIRE(!config.empty());
    REQUIRE(config.find("{") != std::string::npos);
    REQUIRE(config.find("}") != std::string::npos);

    myEngine.finalize();
}

TEST_CASE("config with custom pool") {
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
            "name": "my_custom_pool",
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

    // Should have at least 2 pools
    REQUIRE(myEngine.pools().size() >= 2);

    // Access by index
    auto pool1 = myEngine.pools()[1];
    REQUIRE(pool1.name() == "my_custom_pool");

    myEngine.finalize();
}

TEST_CASE("config with multiple pools") {
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

    REQUIRE(myEngine.pools().size() >= 3);

    // Access by index (use auto to preserve proxy type)
    auto pool0 = myEngine.pools()[0];
    auto pool1 = myEngine.pools()[1];
    auto pool2 = myEngine.pools()[2];

    REQUIRE(pool0.name() == "__primary__");
    REQUIRE(pool1.name() == "pool_1");
    REQUIRE(pool2.name() == "pool_2");

    myEngine.finalize();
}

TEST_CASE("config with custom xstream") {
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
          },
          {
            "name": "my_xstream",
            "scheduler": {
              "type": "basic_wait",
              "pools": [1]
            }
          }
        ]
      }
    }
    )";

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);

    REQUIRE(myEngine.xstreams().size() >= 2);

    auto custom_xs = myEngine.xstreams()[1];
    REQUIRE(custom_xs.name() == "my_xstream");

    myEngine.finalize();
}

TEST_CASE("pool access by index and name") {
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
            "name": "test_pool",
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

    // Access pool by index
    auto pool_by_index = myEngine.pools()[1];
    REQUIRE(pool_by_index.name() == "test_pool");
    REQUIRE(pool_by_index.index() == 1);

    myEngine.finalize();
}

TEST_CASE("xstream access by index and name") {
    const char* config = R"(
    {
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
          },
          {
            "name": "test_xstream",
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
    auto xs_by_index = myEngine.xstreams()[1];
    REQUIRE(xs_by_index.name() == "test_xstream");
    REQUIRE(xs_by_index.index() == 1);

    myEngine.finalize();
}

} // TEST_SUITE
