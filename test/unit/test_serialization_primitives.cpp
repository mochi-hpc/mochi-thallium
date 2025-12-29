/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for primitive type serialization in Thallium
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <limits>

namespace tl = thallium;

TEST_SUITE("Serialization Primitives") {

TEST_CASE("serialize int") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_int", [](const tl::request& req, int val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_int");
    tl::endpoint self_ep = myEngine.lookup(addr);

    SUBCASE("positive int") {
        int result = rpc.on(self_ep)(42);
        REQUIRE(result == 42);
    }

    SUBCASE("negative int") {
        int result = rpc.on(self_ep)(-123);
        REQUIRE(result == -123);
    }

    SUBCASE("zero") {
        int result = rpc.on(self_ep)(0);
        REQUIRE(result == 0);
    }

    SUBCASE("max int") {
        int result = rpc.on(self_ep)(std::numeric_limits<int>::max());
        REQUIRE(result == std::numeric_limits<int>::max());
    }

    SUBCASE("min int") {
        int result = rpc.on(self_ep)(std::numeric_limits<int>::min());
        REQUIRE(result == std::numeric_limits<int>::min());
    }

    myEngine.finalize();
}

TEST_CASE("serialize int64_t") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_int64", [](const tl::request& req, int64_t val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_int64");
    tl::endpoint self_ep = myEngine.lookup(addr);

    int64_t large_value = 9223372036854775807LL;
    int64_t result = rpc.on(self_ep)(large_value);

    REQUIRE(result == large_value);
    myEngine.finalize();
}

TEST_CASE("serialize unsigned types") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_uint", [](const tl::request& req, unsigned int val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_uint");
    tl::endpoint self_ep = myEngine.lookup(addr);

    unsigned int value = 4294967295U;
    unsigned int result = rpc.on(self_ep)(value);

    REQUIRE(result == value);
    myEngine.finalize();
}

TEST_CASE("serialize float double") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    SUBCASE("float") {
        myEngine.define("echo_float", [](const tl::request& req, float val) {
            req.respond(val);
        });

        auto rpc = myEngine.define("echo_float");
        tl::endpoint self_ep = myEngine.lookup(addr);

        float result = rpc.on(self_ep)(3.14f);
        REQUIRE(result == doctest::Approx(3.14f));
    }

    SUBCASE("double") {
        myEngine.define("echo_double", [](const tl::request& req, double val) {
            req.respond(val);
        });

        auto rpc = myEngine.define("echo_double");
        tl::endpoint self_ep = myEngine.lookup(addr);

        double result = rpc.on(self_ep)(2.718281828);
        REQUIRE(result == doctest::Approx(2.718281828));
    }

    myEngine.finalize();
}

TEST_CASE("serialize char") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_char", [](const tl::request& req, char val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_char");
    tl::endpoint self_ep = myEngine.lookup(addr);

    SUBCASE("letter") {
        char result = rpc.on(self_ep)('A');
        REQUIRE(result == 'A');
    }

    SUBCASE("digit") {
        char result = rpc.on(self_ep)('7');
        REQUIRE(result == '7');
    }

    SUBCASE("null char") {
        char result = rpc.on(self_ep)('\0');
        REQUIRE(result == '\0');
    }

    myEngine.finalize();
}

TEST_CASE("serialize bool") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_bool", [](const tl::request& req, bool val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_bool");
    tl::endpoint self_ep = myEngine.lookup(addr);

    SUBCASE("true") {
        bool result = rpc.on(self_ep)(true);
        REQUIRE(result == true);
    }

    SUBCASE("false") {
        bool result = rpc.on(self_ep)(false);
        REQUIRE(result == false);
    }

    myEngine.finalize();
}

TEST_CASE("serialize enum") {
    enum Color { RED = 0, GREEN = 1, BLUE = 2 };

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_enum", [](const tl::request& req, Color val) {
        req.respond(val);
    });

    auto rpc = myEngine.define("echo_enum");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Color result = rpc.on(self_ep)(BLUE);
    REQUIRE(result == BLUE);
    myEngine.finalize();
}

TEST_CASE("serialize multiple primitives") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_multiple", [](const tl::request& req, int i, double d, char c, bool b) {
        req.respond(i, d, c, b);
    });

    auto rpc = myEngine.define("echo_multiple");
    tl::endpoint self_ep = myEngine.lookup(addr);

    auto result = rpc.on(self_ep)(42, 3.14, 'X', true).as<int, double, char, bool>();
    int i = std::get<0>(result);
    double d = std::get<1>(result);
    char c = std::get<2>(result);
    bool b = std::get<3>(result);

    REQUIRE(i == 42);
    REQUIRE(d == doctest::Approx(3.14));
    REQUIRE(c == 'X');
    REQUIRE(b == true);
    myEngine.finalize();
}

TEST_CASE("serialize boundary values") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    SUBCASE("int8_t boundaries") {
        myEngine.define("echo_int8", [](const tl::request& req, int8_t val) {
            req.respond(val);
        });

        auto rpc = myEngine.define("echo_int8");
        tl::endpoint self_ep = myEngine.lookup(addr);

        int8_t max_val = std::numeric_limits<int8_t>::max();
        int8_t min_val = std::numeric_limits<int8_t>::min();

        int8_t result_max = rpc.on(self_ep)(max_val);
        int8_t result_min = rpc.on(self_ep)(min_val);

        REQUIRE(result_max == max_val);
        REQUIRE(result_min == min_val);
    }

    SUBCASE("uint64_t max") {
        myEngine.define("echo_uint64", [](const tl::request& req, uint64_t val) {
            req.respond(val);
        });

        auto rpc = myEngine.define("echo_uint64");
        tl::endpoint self_ep = myEngine.lookup(addr);

        uint64_t max_val = std::numeric_limits<uint64_t>::max();
        uint64_t result = rpc.on(self_ep)(max_val);
        REQUIRE(result == max_val);
    }

    SUBCASE("double special values") {
        myEngine.define("echo_double_special", [](const tl::request& req, double val) {
            req.respond(val);
        });

        auto rpc = myEngine.define("echo_double_special");
        tl::endpoint self_ep = myEngine.lookup(addr);

        double zero = 0.0;
        double neg_zero = -0.0;
        double max_val = std::numeric_limits<double>::max();
        double min_val = std::numeric_limits<double>::min();

        double result_zero = rpc.on(self_ep)(zero);
        REQUIRE(result_zero == zero);

        double result_neg_zero = rpc.on(self_ep)(neg_zero);
        REQUIRE(result_neg_zero == neg_zero);

        double result_max = rpc.on(self_ep)(max_val);
        REQUIRE(result_max == max_val);

        double result_min = rpc.on(self_ep)(min_val);
        REQUIRE(result_min == min_val);
    }

    myEngine.finalize();
}

} // TEST_SUITE
