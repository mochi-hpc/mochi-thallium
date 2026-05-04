/*
 * Copyright (c) 2024 UChicago Argonne, LLC
 * Unit tests for thallium::bulk_buffer and thallium::bulk_buffer_pool.
 */

#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <cstring>
#include <vector>

namespace tl = thallium;

// ============================================================================
// bulk_buffer tests
// ============================================================================

TEST_SUITE("bulk_buffer") {

TEST_CASE("default construction produces null buffer") {
    tl::bulk_buffer<> buf;
    REQUIRE(buf.is_null());
    REQUIRE(buf.size() == 0);
    REQUIRE(buf.data() == nullptr);
    REQUIRE(buf.use_count() == 0);
}

TEST_CASE("construction allocates and registers buffer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_NOTHROW({
        tl::bulk_buffer<> buf(myEngine, 1024, tl::bulk_mode::read_write);
        REQUIRE(!buf.is_null());
        REQUIRE(buf.size() == 1024);
        REQUIRE(buf.data() != nullptr);
        REQUIRE(buf.use_count() == 1);
    });

    myEngine.finalize();
}

TEST_CASE("construction with read_only mode") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> buf(myEngine, 512, tl::bulk_mode::read_only);
    REQUIRE(!buf.is_null());
    REQUIRE(buf.size() == 512);

    myEngine.finalize();
}

TEST_CASE("construction with write_only mode") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> buf(myEngine, 256, tl::bulk_mode::write_only);
    REQUIRE(!buf.is_null());
    REQUIRE(buf.size() == 256);

    myEngine.finalize();
}

TEST_CASE("copy shares the same backing buffer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> buf(myEngine, 1024, tl::bulk_mode::read_write);
    void* orig_ptr = buf.data();
    REQUIRE(buf.use_count() == 1);

    {
        tl::bulk_buffer<> copy = buf;
        REQUIRE(!copy.is_null());
        REQUIRE(copy.data() == orig_ptr);     // same backing memory
        REQUIRE(copy.size() == buf.size());
        REQUIRE(buf.use_count() == 2);
        REQUIRE(copy.use_count() == 2);
    }

    REQUIRE(buf.use_count() == 1);
    REQUIRE(buf.data() == orig_ptr);

    myEngine.finalize();
}

TEST_CASE("move leaves source null") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> buf(myEngine, 1024, tl::bulk_mode::read_write);
    void* orig_ptr = buf.data();

    tl::bulk_buffer<> moved = std::move(buf);
    REQUIRE(buf.is_null());
    REQUIRE(!moved.is_null());
    REQUIRE(moved.data() == orig_ptr);
    REQUIRE(moved.use_count() == 1);

    myEngine.finalize();
}

TEST_CASE("data() is writable and readable") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> buf(myEngine, 64, tl::bulk_mode::read_write);
    char* p = static_cast<char*>(buf.data());
    std::memset(p, 0xAB, 64);

    for(int i = 0; i < 64; ++i)
        REQUIRE(static_cast<unsigned char>(p[i]) == 0xAB);

    myEngine.finalize();
}

TEST_CASE("bulk_buffer receives data via intra-process RDMA") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    const std::size_t BUFSIZE = 128;
    tl::bulk_buffer<> server_buf(myEngine, BUFSIZE, tl::bulk_mode::write_only);

    myEngine.define("bb_recv",
        [&server_buf](const tl::request& req, tl::bulk& remote) {
            // Pull from client into the pre-allocated bulk_buffer.
            // Uses the free function: remote_bulk >> bulk_buffer
            remote.on(req.get_endpoint()) >> server_buf;
            req.respond(0);
        });

    std::vector<char> send_data(BUFSIZE);
    for(std::size_t i = 0; i < BUFSIZE; ++i)
        send_data[i] = static_cast<char>(i & 0xFF);

    std::vector<std::pair<void*, std::size_t>> segs{
        {send_data.data(), send_data.size()}};
    tl::bulk client_bulk = myEngine.expose(segs, tl::bulk_mode::read_only);

    auto rpc        = myEngine.define("bb_recv");
    tl::endpoint ep = myEngine.lookup(addr);
    int ret         = rpc.on(ep)(client_bulk);
    REQUIRE(ret == 0);

    const char* got = static_cast<const char*>(server_buf.data());
    for(std::size_t i = 0; i < BUFSIZE; ++i)
        REQUIRE(got[i] == static_cast<char>(i & 0xFF));

    myEngine.finalize();
}

} // TEST_SUITE("bulk_buffer")

// ============================================================================
// bulk_buffer_pool tests
// ============================================================================

TEST_SUITE("bulk_buffer_pool") {

TEST_CASE("multi-tier pool: size_multiple <= 1 throws") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_THROWS_AS(
        (tl::bulk_buffer_pool<>(myEngine, 3, 2, 64, 1.0f, tl::bulk_mode::write_only)),
        tl::exception);
    REQUIRE_THROWS_AS(
        (tl::bulk_buffer_pool<>(myEngine, 3, 2, 64, 0.5f, tl::bulk_mode::write_only)),
        tl::exception);

    myEngine.finalize();
}

TEST_CASE("single-tier pool: create and destroy") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    REQUIRE_NOTHROW({
        tl::bulk_buffer_pool<> pool(myEngine, 4, 1024, tl::bulk_mode::write_only);
        REQUIRE(pool.max_buffer_size() == 1024);
    });

    myEngine.finalize();
}

TEST_CASE("get() returns a non-null bulk_buffer") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 4, 1024, tl::bulk_mode::write_only);
    tl::bulk_buffer<> buf = pool.get();

    REQUIRE(!buf.is_null());
    REQUIRE(buf.size() == 1024);
    REQUIRE(buf.data() != nullptr);

    myEngine.finalize();
}

TEST_CASE("try_get() returns null when all slots are held") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 2, 512, tl::bulk_mode::write_only);

    tl::bulk_buffer<> a = pool.get();
    tl::bulk_buffer<> b = pool.get();

    tl::bulk_buffer<> c = pool.try_get();
    REQUIRE(c.is_null());

    myEngine.finalize();
}

TEST_CASE("buffer returned to pool when lease is dropped") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 1, 512, tl::bulk_mode::write_only);

    {
        tl::bulk_buffer<> buf = pool.get();
        REQUIRE(!buf.is_null());
        tl::bulk_buffer<> should_fail = pool.try_get();
        REQUIRE(should_fail.is_null());
    }  // buf destroyed → returned to pool

    tl::bulk_buffer<> buf2 = pool.try_get();
    REQUIRE(!buf2.is_null());

    myEngine.finalize();
}

TEST_CASE("shared copies of a lease all hold the pool slot") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 1, 512, tl::bulk_mode::write_only);

    tl::bulk_buffer<> copy;
    {
        tl::bulk_buffer<> buf = pool.get();
        copy = buf;
        REQUIRE(buf.use_count() >= 2);
    }  // buf destroyed, copy still holds a ref → slot still held

    tl::bulk_buffer<> should_fail = pool.try_get();
    REQUIRE(should_fail.is_null());

    copy = tl::bulk_buffer<>();  // last ref dropped → slot returned

    tl::bulk_buffer<> ok = pool.try_get();
    REQUIRE(!ok.is_null());

    myEngine.finalize();
}

TEST_CASE("multi-tier pool: max_buffer_size and tier selection") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // 3 tiers: 64, 256, 1024 bytes; 2 buffers each
    tl::bulk_buffer_pool<> pool(myEngine, 3, 2, 64, 4.0f,
                                 tl::bulk_mode::write_only);

    REQUIRE(pool.max_buffer_size() == 1024);

    // Smallest available tier
    tl::bulk_buffer<> small = pool.get(0);
    REQUIRE(small.size() == 64);

    // Next tier for min_size=100
    tl::bulk_buffer<> medium = pool.get(100);
    REQUIRE(medium.size() == 256);

    // Largest tier for min_size=300
    tl::bulk_buffer<> large = pool.get(300);
    REQUIRE(large.size() == 1024);

    myEngine.finalize();
}

TEST_CASE("pool_state outlives the pool object") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer<> held;
    {
        tl::bulk_buffer_pool<> pool(myEngine, 1, 256, tl::bulk_mode::write_only);
        held = pool.get();
    }  // pool destroyed; pool_state kept alive by held's custom deleter

    REQUIRE(!held.is_null());
    REQUIRE(held.size() == 256);
    REQUIRE(held.data() != nullptr);

    held = tl::bulk_buffer<>();  // release → pool_state finally destroyed

    myEngine.finalize();
}

TEST_CASE("pool move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 2, 512, tl::bulk_mode::write_only);
    tl::bulk_buffer_pool<> moved = std::move(pool);

    REQUIRE(moved.max_buffer_size() == 512);

    tl::bulk_buffer<> buf = moved.get();
    REQUIRE(!buf.is_null());

    myEngine.finalize();
}

TEST_CASE("pool used in intra-process RDMA") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    const std::size_t BUFSIZE = 128;
    tl::bulk_buffer_pool<> pool(myEngine, 4, BUFSIZE, tl::bulk_mode::write_only);

    myEngine.define("pool_recv",
        [&pool](const tl::request& req, tl::bulk& remote) {
            tl::bulk_buffer<> buf = pool.get();
            remote.on(req.get_endpoint()) >> buf;
            // Echo data back as vector
            const char* p = static_cast<const char*>(buf.data());
            std::vector<char> result(p, p + buf.size());
            req.respond(result);
        });

    std::vector<char> send_data(BUFSIZE);
    for(std::size_t i = 0; i < BUFSIZE; ++i)
        send_data[i] = static_cast<char>(i & 0xFF);

    std::vector<std::pair<void*, std::size_t>> segs{
        {send_data.data(), send_data.size()}};
    tl::bulk client_bulk = myEngine.expose(segs, tl::bulk_mode::read_only);

    auto rpc        = myEngine.define("pool_recv");
    tl::endpoint ep = myEngine.lookup(addr);
    std::vector<char> result = rpc.on(ep)(client_bulk);

    REQUIRE(result.size() == BUFSIZE);
    REQUIRE(result == send_data);

    myEngine.finalize();
}

TEST_CASE("extend_if_needed: pool starts empty and grows on demand") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 0, 256, tl::bulk_mode::write_only);

    tl::bulk_buffer<> buf = pool.get(0, true);
    REQUIRE(!buf.is_null());
    REQUIRE(buf.size() == 256);
    REQUIRE(buf.data() != nullptr);

    myEngine.finalize();
}

TEST_CASE("extend_if_needed: selects correct tier") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // 3 tiers: 64, 256, 1024 bytes; 0 buffers each (lazy allocation)
    tl::bulk_buffer_pool<> pool(myEngine, 3, 0, 64, 4.0f,
                                 tl::bulk_mode::write_only);

    tl::bulk_buffer<> small  = pool.get(0,   true);
    tl::bulk_buffer<> medium = pool.get(100, true);
    tl::bulk_buffer<> large  = pool.get(300, true);

    REQUIRE(small.size()  == 64);
    REQUIRE(medium.size() == 256);
    REQUIRE(large.size()  == 1024);

    myEngine.finalize();
}

TEST_CASE("extend_if_needed: extended buffers are returned to pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::bulk_buffer_pool<> pool(myEngine, 0, 128, tl::bulk_mode::write_only);

    {
        tl::bulk_buffer<> buf = pool.get(0, true);
        REQUIRE(!buf.is_null());
    } // lease dropped → buffer returned to free list

    tl::bulk_buffer<> reused = pool.try_get();
    REQUIRE(!reused.is_null());
    REQUIRE(reused.size() == 128);

    myEngine.finalize();
}

TEST_CASE("extend_if_needed: creates new tier when min_size exceeds all buckets") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Pool with a single 64-byte tier.
    tl::bulk_buffer_pool<> pool(myEngine, 1, 64, tl::bulk_mode::write_only);

    // Request 4096 bytes — beyond the existing tier; a new tier must be created
    // at 1.2× the requested size = 4915 bytes.
    const std::size_t requested = 4096;
    const std::size_t expected  = static_cast<std::size_t>(requested * 1.2);
    tl::bulk_buffer<> buf = pool.get(requested, true);
    REQUIRE(!buf.is_null());
    REQUIRE(buf.size() == expected);
    REQUIRE(pool.max_buffer_size() == expected);

    // The buffer is returned to the new tier's free list when the lease drops.
    buf = tl::bulk_buffer<>();
    tl::bulk_buffer<> reused = pool.try_get(requested);
    REQUIRE(!reused.is_null());
    REQUIRE(reused.size() == expected);

    myEngine.finalize();
}

} // TEST_SUITE("bulk_buffer_pool")
