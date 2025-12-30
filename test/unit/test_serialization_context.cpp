/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for context-aware serialization
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

// Simple class that uses a single context parameter (scale factor)
struct ScaledValue {
    double value;

    ScaledValue() : value(0.0) {}
    ScaledValue(double v) : value(v) {}

    template<typename A>
    void serialize(A& ar) {
        double scale = std::get<0>(ar.get_context());
        double scaled = value * scale;
        ar & scaled;
        value = scaled / scale;
    }

    bool operator==(const ScaledValue& other) const {
        return std::abs(value - other.value) < 0.0001;
    }
};

// Class using multiple context parameters (offset and scale)
struct TransformedValue {
    double value;

    TransformedValue() : value(0.0) {}
    TransformedValue(double v) : value(v) {}

    template<typename A>
    void serialize(A& ar) {
        double offset = std::get<0>(ar.get_context());
        double scale = std::get<1>(ar.get_context());
        double transformed = (value + offset) * scale;
        ar & transformed;
        value = (transformed / scale) - offset;
    }

    bool operator==(const TransformedValue& other) const {
        return std::abs(value - other.value) < 0.0001;
    }
};

// Class with context reference (modifies context)
struct CountedValue {
    int value;

    CountedValue() : value(0) {}
    CountedValue(int v) : value(v) {}

    template<typename A>
    void serialize(A& ar) {
        int& counter = std::get<0>(ar.get_context());
        counter++;  // Increment counter each time we serialize
        ar & value;
    }

    bool operator==(const CountedValue& other) const {
        return value == other.value;
    }
};

// Class using mixed types in context
struct MixedContextValue {
    std::string data;

    MixedContextValue() = default;
    MixedContextValue(const std::string& s) : data(s) {}

    template<typename A>
    void serialize(A& ar) {
        const std::string& prefix = std::get<0>(ar.get_context());
        int suffix_num = std::get<1>(ar.get_context());
        std::string modified = prefix + data + std::to_string(suffix_num);
        ar & modified;
        // Remove prefix and suffix to restore original
        if (modified.size() >= prefix.size() + 1) {
            data = modified.substr(prefix.size(),
                                  modified.size() - prefix.size() - std::to_string(suffix_num).size());
        }
    }

    bool operator==(const MixedContextValue& other) const {
        return data == other.data;
    }
};

TEST_SUITE("Serialization Context") {

TEST_CASE("single context parameter") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_scaled", [&myEngine](const tl::request& req) {
        double scale = 2.0;
        ScaledValue input = req.get_input()
                              .with_serialization_context(scale)
                              .as<ScaledValue>();

        req.with_serialization_context(scale).respond(input);
    });

    auto rpc = myEngine.define("echo_scaled");
    tl::endpoint self_ep = myEngine.lookup(addr);

    ScaledValue input(10.0);
    double scale = 2.0;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(scale)
                       (input);
    ScaledValue result = response.with_serialization_context(scale);

    REQUIRE(result == input);
    REQUIRE(result.value == 10.0);

    myEngine.finalize();
}

TEST_CASE("multiple context parameters") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_transformed", [&myEngine](const tl::request& req) {
        double offset = 5.0;
        double scale = 2.0;

        TransformedValue input = req.get_input()
                                   .with_serialization_context(offset, scale)
                                   .as<TransformedValue>();

        req.with_serialization_context(offset, scale).respond(input);
    });

    auto rpc = myEngine.define("echo_transformed");
    tl::endpoint self_ep = myEngine.lookup(addr);

    TransformedValue input(100.0);
    double offset = 5.0;
    double scale = 2.0;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(offset, scale)
                       (input);
    TransformedValue result = response.with_serialization_context(offset, scale);

    REQUIRE(result == input);
    REQUIRE(result.value == 100.0);

    myEngine.finalize();
}

TEST_CASE("context with reference") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_counted", [&myEngine](const tl::request& req) {
        int counter = 0;

        CountedValue input = req.get_input()
                               .with_serialization_context(std::ref(counter))
                               .as<CountedValue>();

        // Counter should have been incremented during deserialization
        int first_count = counter;

        req.with_serialization_context(std::ref(counter)).respond(input);

        // Counter should have been incremented again during serialization
        REQUIRE(counter == first_count + 1);
    });

    auto rpc = myEngine.define("echo_counted");
    tl::endpoint self_ep = myEngine.lookup(addr);

    CountedValue input(42);
    int client_counter = 0;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(std::ref(client_counter))
                       (input);

    // Counter incremented during client-side serialization
    REQUIRE(client_counter == 1);

    CountedValue result = response.with_serialization_context(std::ref(client_counter));

    // Counter incremented again during client-side deserialization
    REQUIRE(client_counter == 2);
    REQUIRE(result == input);
    REQUIRE(result.value == 42);

    myEngine.finalize();
}

TEST_CASE("mixed types in context") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_mixed", [&myEngine](const tl::request& req) {
        std::string prefix = "server_";
        int suffix = 99;

        MixedContextValue input = req.get_input()
                                    .with_serialization_context(prefix, suffix)
                                    .as<MixedContextValue>();

        req.with_serialization_context(prefix, suffix).respond(input);
    });

    auto rpc = myEngine.define("echo_mixed");
    tl::endpoint self_ep = myEngine.lookup(addr);

    MixedContextValue input("test");
    std::string prefix = "server_";
    int suffix = 99;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(prefix, suffix)
                       (input);
    MixedContextValue result = response.with_serialization_context(prefix, suffix);

    REQUIRE(result == input);
    REQUIRE(result.data == "test");

    myEngine.finalize();
}

TEST_CASE("different contexts for input and output") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("transform", [&myEngine](const tl::request& req) {
        double input_scale = 2.0;
        double output_scale = 3.0;

        ScaledValue input = req.get_input()
                              .with_serialization_context(input_scale)
                              .as<ScaledValue>();

        // Create output with different value
        ScaledValue output(input.value * 2.0);

        req.with_serialization_context(output_scale).respond(output);
    });

    auto rpc = myEngine.define("transform");
    tl::endpoint self_ep = myEngine.lookup(addr);

    ScaledValue input(10.0);
    double input_scale = 2.0;
    double output_scale = 3.0;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(input_scale)
                       (input);
    ScaledValue result = response.with_serialization_context(output_scale);

    // Result should be input.value * 2.0 = 20.0
    REQUIRE(result.value == 20.0);

    myEngine.finalize();
}

TEST_CASE("context with multiple values") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_two_scaled", [&myEngine](const tl::request& req) {
        double scale = 2.5;

        auto inputs = req.get_input()
                        .with_serialization_context(scale)
                        .as<ScaledValue, ScaledValue>();

        ScaledValue v1 = std::get<0>(inputs);
        ScaledValue v2 = std::get<1>(inputs);

        req.with_serialization_context(scale).respond(v1, v2);
    });

    auto rpc = myEngine.define("echo_two_scaled");
    tl::endpoint self_ep = myEngine.lookup(addr);

    ScaledValue input1(100.0);
    ScaledValue input2(200.0);
    double scale = 2.5;

    auto response = rpc.on(self_ep)
                       .with_serialization_context(scale)
                       (input1, input2);

    auto results = response.with_serialization_context(scale).as<ScaledValue, ScaledValue>();
    ScaledValue result1 = std::get<0>(results);
    ScaledValue result2 = std::get<1>(results);

    REQUIRE(result1 == input1);
    REQUIRE(result2 == input2);
    REQUIRE(result1.value == 100.0);
    REQUIRE(result2.value == 200.0);

    myEngine.finalize();
}

} // TEST_SUITE
