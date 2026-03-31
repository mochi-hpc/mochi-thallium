/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for thallium::bulk RDMA operations
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <vector>
#include <cstring>

namespace tl = thallium;

TEST_SUITE("Bulk Transfers") {

TEST_CASE("bulk expose single segment") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(1024, 'A');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    REQUIRE_NOTHROW({
        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_write);
    });

    myEngine.finalize();
}

TEST_CASE("bulk expose multiple segments") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer1(512);
    std::vector<char> buffer2(512);
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer1.data(), buffer1.size()},
        {buffer2.data(), buffer2.size()}
    };

    REQUIRE_NOTHROW({
        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_write);
    });

    myEngine.finalize();
}

TEST_CASE("bulk mode read only") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(256, 'X');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);

    REQUIRE_NOTHROW({
        // Bulk object should be valid
        REQUIRE(local.size() == buffer.size());
    });

    myEngine.finalize();
}

TEST_CASE("bulk mode write only") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(256);
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);

    REQUIRE(local.size() == buffer.size());

    myEngine.finalize();
}

TEST_CASE("bulk mode read write") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(256);
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_write);

    REQUIRE(local.size() == buffer.size());

    myEngine.finalize();
}

TEST_CASE("bulk transfer pull") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_pull", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        // Server receives bulk handle and can pull data from it
        std::vector<char> local_buffer(remote_bulk.size());
        std::vector<std::pair<void*, size_t>> segments = {
            {local_buffer.data(), local_buffer.size()}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
        remote_bulk.on(req.get_endpoint()) >> local;

        req.respond(local_buffer);
    });

    // Client side
    std::vector<char> send_buffer(128, 'B');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_pull");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<char> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result.size() == send_buffer.size());
    REQUIRE(result == send_buffer);

    myEngine.finalize();
}

TEST_CASE("bulk transfer push") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_push", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        // Server pushes data to client's bulk
        std::vector<char> local_buffer(remote_bulk.size(), 'Z');
        std::vector<std::pair<void*, size_t>> segments = {
            {local_buffer.data(), local_buffer.size()}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);
        remote_bulk.on(req.get_endpoint()) << local;

        req.respond();
    });

    // Client side - prepare buffer to receive data
    std::vector<char> recv_buffer(128, '\0');
    std::vector<std::pair<void*, size_t>> segments = {
        {recv_buffer.data(), recv_buffer.size()}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::write_only);

    auto rpc = myEngine.define("bulk_push");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)(bulk_handle);

    // Buffer should now contain 'Z' characters
    REQUIRE(recv_buffer[0] == 'Z');
    REQUIRE(recv_buffer[recv_buffer.size() - 1] == 'Z');

    myEngine.finalize();
}

TEST_CASE("bulk size query") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(512);
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);

    REQUIRE(local.size() == 512);

    myEngine.finalize();
}

TEST_CASE("bulk empty") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk empty_bulk;

    // Default constructed bulk should be empty/null
    REQUIRE(empty_bulk.size() == 0);

    myEngine.finalize();
}

TEST_CASE("bulk serialization") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_bulk", [](const tl::request& req, tl::bulk& b) {
        // Just echo back the bulk handle
        req.respond(b);
    });

    std::vector<char> buffer(256, 'M');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("echo_bulk");
    tl::endpoint self_ep = myEngine.lookup(addr);

    tl::bulk result = rpc.on(self_ep)(local);

    REQUIRE(result.size() == local.size());

    myEngine.finalize();
}

TEST_CASE("bulk large transfer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_large", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        std::vector<char> local_buffer(remote_bulk.size());
        std::vector<std::pair<void*, size_t>> segments = {
            {local_buffer.data(), local_buffer.size()}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
        remote_bulk.on(req.get_endpoint()) >> local;

        // Verify data
        bool all_correct = true;
        for (size_t i = 0; i < local_buffer.size(); ++i) {
            if (local_buffer[i] != static_cast<char>(i % 256)) {
                all_correct = false;
                break;
            }
        }

        req.respond(all_correct);
    });

    // Create large buffer with pattern
    std::vector<char> send_buffer(1024 * 1024);  // 1 MB
    for (size_t i = 0; i < send_buffer.size(); ++i) {
        send_buffer[i] = static_cast<char>(i % 256);
    }

    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_large");
    tl::endpoint self_ep = myEngine.lookup(addr);

    bool result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result == true);

    myEngine.finalize();
}

TEST_CASE("bulk multiple segments transfer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_multi_seg", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        std::vector<char> local_buffer(remote_bulk.size());
        std::vector<std::pair<void*, size_t>> segments = {
            {local_buffer.data(), local_buffer.size()}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
        remote_bulk.on(req.get_endpoint()) >> local;

        req.respond(local_buffer);
    });

    // Create multiple segments
    std::vector<char> seg1(100, 'A');
    std::vector<char> seg2(100, 'B');
    std::vector<char> seg3(100, 'C');

    std::vector<std::pair<void*, size_t>> segments = {
        {seg1.data(), seg1.size()},
        {seg2.data(), seg2.size()},
        {seg3.data(), seg3.size()}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_multi_seg");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<char> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result.size() == 300);
    // First 100 should be 'A', next 100 'B', last 100 'C'
    REQUIRE(result[0] == 'A');
    REQUIRE(result[100] == 'B');
    REQUIRE(result[200] == 'C');

    myEngine.finalize();
}

TEST_CASE("bulk with structured data") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_structured", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        std::vector<int> local_buffer(remote_bulk.size() / sizeof(int));
        std::vector<std::pair<void*, size_t>> segments = {
            {local_buffer.data(), local_buffer.size() * sizeof(int)}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
        remote_bulk.on(req.get_endpoint()) >> local;

        req.respond(local_buffer);
    });

    // Send array of integers
    std::vector<int> send_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::pair<void*, size_t>> segments = {
        {send_data.data(), send_data.size() * sizeof(int)}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_structured");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<int> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result.size() == send_data.size());
    REQUIRE(result == send_data);

    myEngine.finalize();
}

TEST_CASE("bulk zero size") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer;  // Empty buffer
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    // Exposing zero-size buffer should work
    tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);

    REQUIRE(local.size() == 0);

    myEngine.finalize();
}

TEST_CASE("bulk copy semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(128, 'D');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk original = myEngine.expose(segments, tl::bulk_mode::read_only);
    tl::bulk copy = original;

    REQUIRE(copy.size() == original.size());

    myEngine.finalize();
}

TEST_CASE("bulk move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buffer(128, 'E');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk original = myEngine.expose(segments, tl::bulk_mode::read_only);
    size_t original_size = original.size();

    tl::bulk moved = std::move(original);

    REQUIRE(moved.size() == original_size);

    myEngine.finalize();
}

TEST_CASE("bulk bidirectional transfer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_swap", [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
        // Read from remote, modify, write back
        std::vector<char> buffer(remote_bulk.size());
        std::vector<std::pair<void*, size_t>> segments = {
            {buffer.data(), buffer.size()}
        };

        tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_write);

        // Pull from remote
        remote_bulk.on(req.get_endpoint()) >> local;

        // Modify data
        for (auto& c : buffer) {
            c = c + 1;
        }

        // Push back to remote
        remote_bulk.on(req.get_endpoint()) << local;

        req.respond();
    });

    std::vector<char> buffer(64, 'A');
    std::vector<std::pair<void*, size_t>> segments = {
        {buffer.data(), buffer.size()}
    };

    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_write);

    auto rpc = myEngine.define("bulk_swap");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)(bulk_handle);

    // Buffer should now contain 'B' (A + 1)
    REQUIRE(buffer[0] == 'B');

    myEngine.finalize();
}

TEST_CASE("bulk move-assignment operator") {
    // Test move-assignment of bulk objects
    // Covers bulk.hpp lines 169-180
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buf1(128, 'A');
    std::vector<char> buf2(256, 'B');

    tl::bulk b1 = myEngine.expose({{buf1.data(), buf1.size()}}, tl::bulk_mode::read_only);
    tl::bulk b2 = myEngine.expose({{buf2.data(), buf2.size()}}, tl::bulk_mode::read_only);

    size_t orig_size = b2.size();

    // Move-assign - Lines 169-180
    b1 = std::move(b2);

    REQUIRE(b1.size() == orig_size);
    REQUIRE(b2.is_null());

    myEngine.finalize();
}

TEST_CASE("bulk self move-assignment") {
    // Test self-assignment safety in move-assignment operator
    // Covers bulk.hpp lines 169-171 (self-assignment guard)
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::vector<char> buf(64, 'X');
    tl::bulk b = myEngine.expose({{buf.data(), buf.size()}}, tl::bulk_mode::read_only);

    size_t orig_size = b.size();

    // Self-assignment should be safe - Lines 169-171
    b = std::move(b);

    REQUIRE(b.size() == orig_size);

    myEngine.finalize();
}

TEST_CASE("bulk timed transfer pull success") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_timed_pull",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size());
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            // Use timed transfer with a generous deadline (5 seconds)
            std::size_t n = remote_bulk.on(req.get_endpoint()).timed(5000.0) >> local;
            REQUIRE(n == local_buffer.size());
            req.respond(local_buffer);
        });

    std::vector<char> send_buffer(128, 'T');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_timed_pull");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<char> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result.size() == send_buffer.size());
    REQUIRE(result == send_buffer);

    myEngine.finalize();
}

TEST_CASE("bulk timed transfer push success") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_timed_push",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size(), 'P');
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);
            // Use timed transfer with a generous deadline (5 seconds)
            std::size_t n = remote_bulk.on(req.get_endpoint()).timed(5000.0) << local;
            REQUIRE(n == local_buffer.size());
            req.respond();
        });

    std::vector<char> recv_buffer(128, '\0');
    std::vector<std::pair<void*, size_t>> segments = {
        {recv_buffer.data(), recv_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::write_only);

    auto rpc = myEngine.define("bulk_timed_push");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)(bulk_handle);

    REQUIRE(recv_buffer[0] == 'P');
    REQUIRE(recv_buffer[recv_buffer.size() - 1] == 'P');

    myEngine.finalize();
}

TEST_CASE("bulk async timed transfer pull success") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_async_timed_pull",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size());
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            tl::async_bulk_op op =
                remote_bulk.on(req.get_endpoint()).timed(5000.0).pull_to(local);
            std::size_t n = op.wait();
            REQUIRE(n == local_buffer.size());
            req.respond(local_buffer);
        });

    std::vector<char> send_buffer(128, 'Q');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_async_timed_pull");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<char> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result.size() == send_buffer.size());
    REQUIRE(result == send_buffer);

    myEngine.finalize();
}

TEST_CASE("bulk async timed transfer push success") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_async_timed_push",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size(), 'R');
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);
            tl::async_bulk_op op =
                remote_bulk.on(req.get_endpoint()).timed(5000.0).push_from(local);
            std::size_t n = op.wait();
            REQUIRE(n == local_buffer.size());
            req.respond();
        });

    std::vector<char> recv_buffer(128, '\0');
    std::vector<std::pair<void*, size_t>> segments = {
        {recv_buffer.data(), recv_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::write_only);

    auto rpc = myEngine.define("bulk_async_timed_push");
    tl::endpoint self_ep = myEngine.lookup(addr);

    rpc.on(self_ep)(bulk_handle);

    REQUIRE(recv_buffer[0] == 'R');
    REQUIRE(recv_buffer[recv_buffer.size() - 1] == 'R');

    myEngine.finalize();
}

TEST_CASE("bulk timed transfer chrono duration") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_timed_chrono",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size());
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            // Use std::chrono::duration overload
            std::size_t n =
                remote_bulk.on(req.get_endpoint()).timed(std::chrono::seconds(5)) >> local;
            REQUIRE(n == local_buffer.size());
            req.respond(local_buffer);
        });

    std::vector<char> send_buffer(64, 'C');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_timed_chrono");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<char> result = rpc.on(self_ep)(bulk_handle);

    REQUIRE(result == send_buffer);

    myEngine.finalize();
}

TEST_CASE("bulk timed transfer timeout exception") {
    // Tests that margo_bulk_transfer_timed propagates HG_TIMEOUT as tl::timeout.
    // A near-zero deadline may or may not fire on shared-memory transports;
    // both success and tl::timeout are acceptable outcomes.
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_timed_timeout_sync",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size());
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            try {
                remote_bulk.on(req.get_endpoint()).timed(0.000001) >> local;
                req.respond(true);  // completed before timeout
            } catch(const tl::timeout&) {
                req.respond(false); // timed out as expected
            }
        });

    std::vector<char> send_buffer(128, 'X');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_timed_timeout_sync");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Either the transfer succeeded before the deadline, or tl::timeout was thrown.
    // Both are valid — what must NOT happen is any other exception type.
    bool completed = rpc.on(self_ep)(bulk_handle);
    (void)completed;

    myEngine.finalize();
}

TEST_CASE("bulk async timed transfer timeout exception") {
    // Tests that margo_bulk_itransfer_timed propagates HG_TIMEOUT via wait()
    // as tl::timeout.
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("bulk_timed_timeout_async",
        [&myEngine](const tl::request& req, tl::bulk& remote_bulk) {
            std::vector<char> local_buffer(remote_bulk.size());
            std::vector<std::pair<void*, size_t>> segments = {
                {local_buffer.data(), local_buffer.size()}
            };
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            try {
                tl::async_bulk_op op =
                    remote_bulk.on(req.get_endpoint()).timed(0.000001).pull_to(local);
                op.wait();
                req.respond(true);  // completed before timeout
            } catch(const tl::timeout&) {
                req.respond(false); // timed out as expected
            }
        });

    std::vector<char> send_buffer(128, 'Y');
    std::vector<std::pair<void*, size_t>> segments = {
        {send_buffer.data(), send_buffer.size()}
    };
    tl::bulk bulk_handle = myEngine.expose(segments, tl::bulk_mode::read_only);

    auto rpc = myEngine.define("bulk_timed_timeout_async");
    tl::endpoint self_ep = myEngine.lookup(addr);

    bool completed = rpc.on(self_ep)(bulk_handle);
    (void)completed;

    myEngine.finalize();
}

} // TEST_SUITE
