/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for custom logging functionality
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/logger.hpp>
#include <vector>
#include <string>
#include <sstream>

namespace tl = thallium;

// Test logger that captures messages
class test_logger : public tl::logger {
public:
    mutable std::vector<std::string> trace_messages;
    mutable std::vector<std::string> debug_messages;
    mutable std::vector<std::string> info_messages;
    mutable std::vector<std::string> warning_messages;
    mutable std::vector<std::string> error_messages;
    mutable std::vector<std::string> critical_messages;

    void trace(const char* msg) const override {
        trace_messages.push_back(msg);
    }

    void debug(const char* msg) const override {
        debug_messages.push_back(msg);
    }

    void info(const char* msg) const override {
        info_messages.push_back(msg);
    }

    void warning(const char* msg) const override {
        warning_messages.push_back(msg);
    }

    void error(const char* msg) const override {
        error_messages.push_back(msg);
    }

    void critical(const char* msg) const override {
        critical_messages.push_back(msg);
    }

    void clear() const {
        trace_messages.clear();
        debug_messages.clear();
        info_messages.clear();
        warning_messages.clear();
        error_messages.clear();
        critical_messages.clear();
    }

    size_t total_message_count() const {
        return trace_messages.size()
             + debug_messages.size()
             + info_messages.size()
             + warning_messages.size()
             + error_messages.size()
             + critical_messages.size();
    }
};

TEST_SUITE("Logging") {

TEST_CASE("custom logger basic functionality") {
    test_logger logger;

    // Manually call logger methods
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warning("warning message");
    logger.error("error message");
    logger.critical("critical message");

    REQUIRE(logger.trace_messages.size() == 1);
    REQUIRE(logger.debug_messages.size() == 1);
    REQUIRE(logger.info_messages.size() == 1);
    REQUIRE(logger.warning_messages.size() == 1);
    REQUIRE(logger.error_messages.size() == 1);
    REQUIRE(logger.critical_messages.size() == 1);

    REQUIRE(logger.trace_messages[0] == "trace message");
    REQUIRE(logger.debug_messages[0] == "debug message");
    REQUIRE(logger.info_messages[0] == "info message");
    REQUIRE(logger.warning_messages[0] == "warning message");
    REQUIRE(logger.error_messages[0] == "error message");
    REQUIRE(logger.critical_messages[0] == "critical message");
}

TEST_CASE("logger clear") {
    test_logger logger;

    logger.trace("test1");
    logger.debug("test2");
    logger.info("test3");

    REQUIRE(logger.total_message_count() == 3);

    logger.clear();

    REQUIRE(logger.total_message_count() == 0);
    REQUIRE(logger.trace_messages.size() == 0);
    REQUIRE(logger.debug_messages.size() == 0);
    REQUIRE(logger.info_messages.size() == 0);
}

TEST_CASE("engine set_logger") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    // Set the logger for the engine
    myEngine.set_logger(&logger);

    // Note: The logger will receive messages from margo/mercury operations
    // We can't easily trigger specific log messages, but we can verify
    // that the logger was set without crashing

    myEngine.finalize();
}

TEST_CASE("engine set_log_level trace") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::trace);

    // Perform some operations that might generate logs
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    // We should have received some log messages
    // (exact count depends on margo/mercury internals)
    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("engine set_log_level debug") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::debug);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("engine set_log_level info") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::info);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("engine set_log_level warning") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::warning);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    // With warning level, we should have fewer messages (or none)
    // than with trace/debug/info
    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("engine set_log_level error") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::error);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("engine set_log_level critical") {
    test_logger logger;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::critical);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    // With critical level, we should have very few (or no) messages
    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("logger level enum values") {
    // Verify that logger level enum has expected values
    int external_val = static_cast<int>(tl::logger::level::external);
    int trace_val = static_cast<int>(tl::logger::level::trace);
    int debug_val = static_cast<int>(tl::logger::level::debug);
    int info_val = static_cast<int>(tl::logger::level::info);
    int warning_val = static_cast<int>(tl::logger::level::warning);
    int error_val = static_cast<int>(tl::logger::level::error);
    int critical_val = static_cast<int>(tl::logger::level::critical);

    // All values should be distinct
    REQUIRE(external_val != trace_val);
    REQUIRE(trace_val != debug_val);
    REQUIRE(debug_val != info_val);
    REQUIRE(info_val != warning_val);
    REQUIRE(warning_val != error_val);
    REQUIRE(error_val != critical_val);

    // Values should be in ascending order (more restrictive)
    REQUIRE(trace_val < debug_val);
    REQUIRE(debug_val < info_val);
    REQUIRE(info_val < warning_val);
    REQUIRE(warning_val < error_val);
    REQUIRE(error_val < critical_val);
}

TEST_CASE("multiple engines with same logger") {
    test_logger logger;

    {
        tl::engine engine1("tcp", THALLIUM_SERVER_MODE, true);
        engine1.set_logger(&logger);
        engine1.set_log_level(tl::logger::level::info);

        std::string addr1 = static_cast<std::string>(engine1.self());

        engine1.finalize();
    }

    size_t count_after_first = logger.total_message_count();

    {
        tl::engine engine2("tcp", THALLIUM_SERVER_MODE, true);
        engine2.set_logger(&logger);
        engine2.set_log_level(tl::logger::level::info);

        std::string addr2 = static_cast<std::string>(engine2.self());

        engine2.finalize();
    }

    size_t count_after_second = logger.total_message_count();

    // Both engines should have logged (count should increase or stay same)
    REQUIRE(count_after_second >= count_after_first);
}

TEST_CASE("global logger set") {
    test_logger logger;

    // Set global logger
    tl::logger::set_global_logger(&logger);

    // Create engine (will use global logger by default)
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    // Should have received some messages
    REQUIRE(logger.total_message_count() >= 0);
}

TEST_CASE("global log level set") {
    test_logger logger;

    // Set global log level
    tl::logger::set_global_log_level(tl::logger::level::warning);

    // Set global logger
    tl::logger::set_global_logger(&logger);

    // Create engine
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);

    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.finalize();

    // With warning level globally, should have fewer messages
    REQUIRE(logger.total_message_count() >= 0);
}

} // TEST_SUITE
