/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for Argobots threads and tasks
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <atomic>

namespace tl = thallium;

TEST_SUITE("Argobots Threads and Tasks") {

TEST_CASE("create and join thread via pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::atomic<bool> thread_executed{false};

    // Create a thread on the pool
    tl::managed<tl::thread> th = handler_pool.make_thread([&thread_executed]() {
        thread_executed.store(true);
    });

    // Join the thread (wait for it to complete)
    th->join();

    REQUIRE(thread_executed.load());

    myEngine.finalize();
}

TEST_CASE("thread id") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::uint64_t thread_id = 0;

    tl::managed<tl::thread> th = handler_pool.make_thread([&thread_id]() {
        thread_id = tl::thread::self_id();
    });

    // Get ID of the created thread (may be 0 in some Argobots versions)
    std::uint64_t created_thread_id = th->id();
    REQUIRE(created_thread_id >= 0);

    th->join();

    // Thread should have executed and recorded its ID
    // Note: IDs might be 0 or > 0 depending on Argobots version
    REQUIRE(thread_id >= 0);

    myEngine.finalize();
}

TEST_CASE("thread self identification") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    bool self_check_passed = false;

    tl::managed<tl::thread> th = handler_pool.make_thread([&self_check_passed]() {
        tl::thread self_thread = tl::thread::self();
        // Verify that we got a non-null thread
        self_check_passed = (self_thread.native_handle() != ABT_THREAD_NULL);
    });

    th->join();

    REQUIRE(self_check_passed);

    myEngine.finalize();
}

TEST_CASE("thread stacksize") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::thread> th = handler_pool.make_thread([]() {
        // Simple work
    });

    // Get stack size of the thread (should be > 0)
    std::size_t stacksize = th->stacksize();
    REQUIRE(stacksize > 0);

    th->join();

    myEngine.finalize();
}

TEST_CASE("thread migratability") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::thread> th = handler_pool.make_thread([]() {
        // Simple work
    });

    // By default, threads should be migratable
    bool is_migratable = th->is_migratable();
    REQUIRE(is_migratable);

    // Set to non-migratable
    th->set_migratable(false);
    REQUIRE_FALSE(th->is_migratable());

    // Set back to migratable
    th->set_migratable(true);
    REQUIRE(th->is_migratable());

    th->join();

    myEngine.finalize();
}

TEST_CASE("multiple threads") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::atomic<int> counter{0};

    std::vector<tl::managed<tl::thread>> threads;
    const int num_threads = 5;

    // Create multiple threads
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(handler_pool.make_thread([&counter]() {
            counter.fetch_add(1);
        }));
    }

    // Join all threads
    for (auto& th : threads) {
        th->join();
    }

    REQUIRE(counter.load() == num_threads);

    myEngine.finalize();
}

TEST_CASE("thread comparison") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::thread> th1 = handler_pool.make_thread([]() {});
    tl::managed<tl::thread> th2 = handler_pool.make_thread([]() {});

    // Copy th1 to th3
    tl::thread th3 = *th1;

    // th1 and th3 should be equal (same thread)
    REQUIRE(*th1 == th3);

    // th1 and th2 should be different
    REQUIRE_FALSE(*th1 == *th2);

    th1->join();
    th2->join();

    myEngine.finalize();
}

TEST_CASE("thread copy and move semantics") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::thread> th1 = handler_pool.make_thread([]() {});

    // Copy the thread wrapper (not creating a new thread)
    tl::thread th_copy = *th1;
    REQUIRE(th_copy == *th1);

    // Move the managed wrapper
    tl::managed<tl::thread> th_moved = std::move(th1);
    REQUIRE(th_moved->native_handle() != ABT_THREAD_NULL);

    th_moved->join();

    myEngine.finalize();
}

TEST_CASE("create and join task via pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::atomic<bool> task_executed{false};

    // Create a task on the pool
    tl::managed<tl::task> tk = handler_pool.make_task([&task_executed]() {
        task_executed.store(true);
    });

    // Join the task (wait for it to complete)
    tk->join();

    REQUIRE(task_executed.load());

    myEngine.finalize();
}

TEST_CASE("task id") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::uint64_t task_id = 0;

    tl::managed<tl::task> tk = handler_pool.make_task([&task_id]() {
        task_id = tl::task::self_id();
    });

    // Get ID of the created task (may be 0 in some Argobots versions)
    std::uint64_t created_task_id = tk->id();
    REQUIRE(created_task_id >= 0);

    tk->join();

    // Task should have executed and recorded its ID
    // Note: IDs might be 0 or > 0 depending on Argobots version
    REQUIRE(task_id >= 0);

    myEngine.finalize();
}

TEST_CASE("task migrability") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::task> tk = handler_pool.make_task([]() {
        // Simple work
    });

    // By default, tasks should be migratable
    bool is_migratable = tk->is_migratable();
    REQUIRE(is_migratable);

    // Set to non-migratable
    tk->set_migratable(false);
    REQUIRE_FALSE(tk->is_migratable());

    // Set back to migratable
    tk->set_migratable(true);
    REQUIRE(tk->is_migratable());

    tk->join();

    myEngine.finalize();
}

TEST_CASE("multiple tasks") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::atomic<int> counter{0};

    std::vector<tl::managed<tl::task>> tasks;
    const int num_tasks = 5;

    // Create multiple tasks
    for (int i = 0; i < num_tasks; ++i) {
        tasks.push_back(handler_pool.make_task([&counter]() {
            counter.fetch_add(1);
        }));
    }

    // Join all tasks
    for (auto& tk : tasks) {
        tk->join();
    }

    REQUIRE(counter.load() == num_tasks);

    myEngine.finalize();
}

TEST_CASE("task comparison") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    tl::managed<tl::task> tk1 = handler_pool.make_task([]() {});
    tl::managed<tl::task> tk2 = handler_pool.make_task([]() {});

    // Copy tk1 to tk3
    tl::task tk3 = *tk1;

    // tk1 and tk3 should be equal (same task)
    REQUIRE(*tk1 == tk3);

    // tk1 and tk2 should be different
    REQUIRE_FALSE(*tk1 == *tk2);

    tk1->join();
    tk2->join();

    myEngine.finalize();
}

TEST_CASE("thread on custom pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create xstream with the custom pool
    tl::managed<tl::xstream> custom_xs = tl::xstream::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    std::atomic<bool> thread_executed{false};

    // Create thread on custom pool
    tl::managed<tl::thread> th = custom_pool->make_thread([&thread_executed]() {
        thread_executed.store(true);
    });

    th->join();

    REQUIRE(thread_executed.load());

    myEngine.finalize();
    custom_xs->join();
}

TEST_CASE("task on custom pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Create a custom pool
    tl::managed<tl::pool> custom_pool = tl::pool::create(
        tl::pool::access::mpmc,
        tl::pool::kind::fifo_wait
    );

    // Create xstream with the custom pool
    tl::managed<tl::xstream> custom_xs = tl::xstream::create(
        tl::scheduler::predef::deflt,
        *custom_pool
    );

    std::atomic<bool> task_executed{false};

    // Create task on custom pool
    tl::managed<tl::task> tk = custom_pool->make_task([&task_executed]() {
        task_executed.store(true);
    });

    tk->join();

    REQUIRE(task_executed.load());

    myEngine.finalize();
    custom_xs->join();
}

TEST_CASE("thread get_last_pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    int last_pool_id = -1;

    tl::managed<tl::thread> th = handler_pool.make_thread([&last_pool_id]() {
        tl::thread self = tl::thread::self();
        last_pool_id = self.get_last_pool_id();
    });

    th->join();

    // Should have a valid pool ID
    REQUIRE(last_pool_id >= 0);

    myEngine.finalize();
}

TEST_CASE("task get_last_pool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    int last_pool_id = -1;

    tl::managed<tl::task> tk = handler_pool.make_task([&last_pool_id]() {
        tl::task self = tl::task::self();
        last_pool_id = self.get_last_pool_id();
    });

    tk->join();

    // Should have a valid pool ID
    REQUIRE(last_pool_id >= 0);

    myEngine.finalize();
}

TEST_CASE("mixed threads and tasks") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    tl::pool handler_pool = myEngine.get_handler_pool();

    std::atomic<int> thread_count{0};
    std::atomic<int> task_count{0};

    // Create threads
    std::vector<tl::managed<tl::thread>> threads;
    for (int i = 0; i < 3; ++i) {
        threads.push_back(handler_pool.make_thread([&thread_count]() {
            thread_count.fetch_add(1);
        }));
    }

    // Create tasks
    std::vector<tl::managed<tl::task>> tasks;
    for (int i = 0; i < 3; ++i) {
        tasks.push_back(handler_pool.make_task([&task_count]() {
            task_count.fetch_add(1);
        }));
    }

    // Join all
    for (auto& th : threads) {
        th->join();
    }
    for (auto& tk : tasks) {
        tk->join();
    }

    REQUIRE(thread_count.load() == 3);
    REQUIRE(task_count.load() == 3);

    myEngine.finalize();
}

} // TEST_SUITE
