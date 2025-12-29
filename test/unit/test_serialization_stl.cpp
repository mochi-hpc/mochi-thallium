/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for STL container serialization in Thallium
 */

#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/complex.hpp>
#include <thallium/serialization/stl/deque.hpp>
#include <thallium/serialization/stl/forward_list.hpp>
#include <thallium/serialization/stl/list.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/multimap.hpp>
#include <thallium/serialization/stl/multiset.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/set.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/unordered_multimap.hpp>
#include <thallium/serialization/stl/unordered_multiset.hpp>
#include <thallium/serialization/stl/unordered_set.hpp>
#include <thallium/serialization/stl/vector.hpp>

namespace tl = thallium;

TEST_SUITE("Serialization STL") {

TEST_CASE("serialize vector") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_vector", [](const tl::request& req, const std::vector<int>& v) {
        req.respond(v);
    });

    auto rpc = myEngine.define("echo_vector");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize list") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_list", [](const tl::request& req, const std::list<int>& l) {
        req.respond(l);
    });

    auto rpc = myEngine.define("echo_list");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::list<int> input = {10, 20, 30, 40};
    std::list<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize deque") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_deque", [](const tl::request& req, const std::deque<int>& d) {
        req.respond(d);
    });

    auto rpc = myEngine.define("echo_deque");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::deque<int> input = {5, 10, 15};
    std::deque<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize forward_list") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_forward_list", [](const tl::request& req, const std::forward_list<int>& fl) {
        req.respond(fl);
    });

    auto rpc = myEngine.define("echo_forward_list");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::forward_list<int> input = {1, 2, 3};
    std::forward_list<int> result = rpc.on(self_ep)(input);

    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize set") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_set", [](const tl::request& req, const std::set<int>& s) {
        req.respond(s);
    });

    auto rpc = myEngine.define("echo_set");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::set<int> input = {3, 1, 4, 1, 5};  // Duplicates will be removed
    std::set<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize multiset") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_multiset", [](const tl::request& req, const std::multiset<int>& ms) {
        req.respond(ms);
    });

    auto rpc = myEngine.define("echo_multiset");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::multiset<int> input = {1, 2, 2, 3, 3, 3};
    std::multiset<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize unordered_set") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_unordered_set", [](const tl::request& req, const std::unordered_set<int>& us) {
        req.respond(us);
    });

    auto rpc = myEngine.define("echo_unordered_set");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::unordered_set<int> input = {5, 2, 8, 1};
    std::unordered_set<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize unordered_multiset") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_unordered_multiset",
        [](const tl::request& req, const std::unordered_multiset<int>& ums) {
        req.respond(ums);
    });

    auto rpc = myEngine.define("echo_unordered_multiset");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::unordered_multiset<int> input = {1, 1, 2, 3, 3, 3};
    std::unordered_multiset<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize map") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_map", [](const tl::request& req, const std::map<std::string, int>& m) {
        req.respond(m);
    });

    auto rpc = myEngine.define("echo_map");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::map<std::string, int> input = {{"one", 1}, {"two", 2}, {"three", 3}};
    std::map<std::string, int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize multimap") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_multimap",
        [](const tl::request& req, const std::multimap<int, std::string>& mm) {
        req.respond(mm);
    });

    auto rpc = myEngine.define("echo_multimap");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::multimap<int, std::string> input = {{1, "a"}, {1, "b"}, {2, "c"}};
    std::multimap<int, std::string> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize unordered_map") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_unordered_map",
        [](const tl::request& req, const std::unordered_map<int, std::string>& um) {
        req.respond(um);
    });

    auto rpc = myEngine.define("echo_unordered_map");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::unordered_map<int, std::string> input = {{1, "one"}, {2, "two"}, {3, "three"}};
    std::unordered_map<int, std::string> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize unordered_multimap") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_unordered_multimap",
        [](const tl::request& req, const std::unordered_multimap<int, int>& umm) {
        req.respond(umm);
    });

    auto rpc = myEngine.define("echo_unordered_multimap");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::unordered_multimap<int, int> input = {{1, 10}, {1, 20}, {2, 30}};
    std::unordered_multimap<int, int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize string") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_string", [](const tl::request& req, const std::string& s) {
        req.respond(s);
    });

    auto rpc = myEngine.define("echo_string");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::string input = "Hello, Thallium!";
    std::string result = rpc.on(self_ep)(input);

    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize pair") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_pair",
        [](const tl::request& req, const std::pair<int, std::string>& p) {
        req.respond(p);
    });

    auto rpc = myEngine.define("echo_pair");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::pair<int, std::string> input = {42, "answer"};
    std::pair<int, std::string> result = rpc.on(self_ep)(input);

    REQUIRE(result.first == input.first);
    REQUIRE(result.second == input.second);

    myEngine.finalize();
}

TEST_CASE("serialize tuple") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_tuple",
        [](const tl::request& req, const std::tuple<int, double, std::string>& t) {
        req.respond(t);
    });

    auto rpc = myEngine.define("echo_tuple");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::tuple<int, double, std::string> input = std::make_tuple(42, 3.14, "test");
    std::tuple<int, double, std::string> result = rpc.on(self_ep)(input);

    REQUIRE(std::get<0>(result) == std::get<0>(input));
    REQUIRE(std::get<1>(result) == doctest::Approx(std::get<1>(input)));
    REQUIRE(std::get<2>(result) == std::get<2>(input));

    myEngine.finalize();
}

TEST_CASE("serialize array") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_array", [](const tl::request& req, const std::array<int, 5>& a) {
        req.respond(a);
    });

    auto rpc = myEngine.define("echo_array");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::array<int, 5> input = {1, 2, 3, 4, 5};
    std::array<int, 5> result = rpc.on(self_ep)(input);

    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize complex") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_complex",
        [](const tl::request& req, const std::complex<double>& c) {
        req.respond(c);
    });

    auto rpc = myEngine.define("echo_complex");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::complex<double> input(3.0, 4.0);
    std::complex<double> result = rpc.on(self_ep)(input);

    REQUIRE(result.real() == doctest::Approx(input.real()));
    REQUIRE(result.imag() == doctest::Approx(input.imag()));

    myEngine.finalize();
}

TEST_CASE("serialize nested containers") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_nested",
        [](const tl::request& req, const std::vector<std::vector<int>>& v) {
        req.respond(v);
    });

    auto rpc = myEngine.define("echo_nested");
    tl::endpoint self_ep = myEngine.lookup(addr);

    std::vector<std::vector<int>> input = {{1, 2}, {3, 4, 5}, {6}};
    std::vector<std::vector<int>> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("serialize empty containers") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    SUBCASE("empty vector") {
        myEngine.define("echo_empty_vector", [](const tl::request& req, const std::vector<int>& v) {
            req.respond(v);
        });

        auto rpc = myEngine.define("echo_empty_vector");
        tl::endpoint self_ep = myEngine.lookup(addr);

        std::vector<int> input;
        std::vector<int> result = rpc.on(self_ep)(input);

        REQUIRE(result.empty());
        REQUIRE(result.size() == 0);
    }

    SUBCASE("empty string") {
        myEngine.define("echo_empty_string", [](const tl::request& req, const std::string& s) {
            req.respond(s);
        });

        auto rpc = myEngine.define("echo_empty_string");
        tl::endpoint self_ep = myEngine.lookup(addr);

        std::string input = "";
        std::string result = rpc.on(self_ep)(input);

        REQUIRE(result.empty());
    }

    SUBCASE("empty map") {
        myEngine.define("echo_empty_map",
            [](const tl::request& req, const std::map<int, int>& m) {
            req.respond(m);
        });

        auto rpc = myEngine.define("echo_empty_map");
        tl::endpoint self_ep = myEngine.lookup(addr);

        std::map<int, int> input;
        std::map<int, int> result = rpc.on(self_ep)(input);

        REQUIRE(result.empty());
    }

    myEngine.finalize();
}

TEST_CASE("serialize large containers") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_large_vector", [](const tl::request& req, const std::vector<int>& v) {
        req.respond(v);
    });

    auto rpc = myEngine.define("echo_large_vector");
    tl::endpoint self_ep = myEngine.lookup(addr);

    // Create a large vector
    std::vector<int> input(10000);
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<int>(i);
    }

    std::vector<int> result = rpc.on(self_ep)(input);

    REQUIRE(result.size() == input.size());
    REQUIRE(result == input);

    myEngine.finalize();
}

} // TEST_SUITE
