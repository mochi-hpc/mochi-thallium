/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Helper utilities and fixtures for Thallium unit tests
 */

#ifndef THALLIUM_TEST_HELPERS_HPP
#define THALLIUM_TEST_HELPERS_HPP

#include <thallium.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <vector>
#include <cstdlib>

namespace tl = thallium;

/**
 * TimeoutGuard: Ensures tests don't hang indefinitely
 * If not destroyed within the timeout period, terminates the process
 */
class TimeoutGuard {
public:
    explicit TimeoutGuard(std::chrono::seconds timeout = std::chrono::seconds(30))
        : timeout_(timeout), armed_(true) {
        timeout_thread_ = std::thread([this]() {
            auto start = std::chrono::steady_clock::now();
            while (armed_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (std::chrono::steady_clock::now() - start > timeout_) {
                    if (armed_.load()) {
                        std::cerr << "TEST TIMEOUT after " << timeout_.count() << " seconds!\n";
                        std::abort();
                    }
                }
            }
        });
    }

    ~TimeoutGuard() {
        armed_.store(false);
        if (timeout_thread_.joinable()) {
            timeout_thread_.join();
        }
    }

private:
    std::chrono::seconds timeout_;
    std::atomic<bool> armed_;
    std::thread timeout_thread_;
};


/**
 * Utility macros for common test patterns
 */

// Execute expression with timeout protection
#define REQUIRE_NO_THROW_TIMEOUT(expr, timeout_sec) \
    do { \
        TimeoutGuard guard(std::chrono::seconds(timeout_sec)); \
        REQUIRE_NOTHROW(expr); \
    } while(0)

// Ensure RPC call succeeds
#define REQUIRE_RPC_SUCCESS(rpc_call) \
    REQUIRE_NOTHROW(rpc_call)

// Check that two endpoints are equal
#define REQUIRE_ENDPOINT_EQUAL(ep1, ep2) \
    REQUIRE(static_cast<std::string>(ep1) == static_cast<std::string>(ep2))

/**
 * Utility functions
 */

// Generate unique RPC name for test isolation
inline std::string generate_rpc_name(const std::string& base) {
    static std::atomic<int> counter{0};
    return base + "_" + std::to_string(counter.fetch_add(1));
}

// Wait for condition with timeout
template<typename Predicate>
bool wait_for_condition(Predicate pred, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

#endif // THALLIUM_TEST_HELPERS_HPP
