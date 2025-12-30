/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 * Unit tests for custom class serialization
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "test_helpers.hpp"
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>

namespace tl = thallium;

// Simple POD struct
struct Point2D {
    double x;
    double y;

    Point2D() : x(0.0), y(0.0) {}
    Point2D(double a, double b) : x(a), y(b) {}

    template<typename A>
    void serialize(A& ar) {
        ar & x;
        ar & y;
    }

    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }
};

// 3D point with default constructor
struct Point3D {
    double x, y, z;

    Point3D() : x(0.0), y(0.0), z(0.0) {}
    Point3D(double a, double b, double c) : x(a), y(b), z(c) {}

    template<typename A>
    void serialize(A& ar) {
        ar & x & y & z;
    }

    bool operator==(const Point3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Nested structure
struct Rectangle {
    Point2D top_left;
    Point2D bottom_right;

    Rectangle() = default;
    Rectangle(Point2D tl, Point2D br) : top_left(tl), bottom_right(br) {}

    template<typename A>
    void serialize(A& ar) {
        ar & top_left;
        ar & bottom_right;
    }

    bool operator==(const Rectangle& other) const {
        return top_left == other.top_left && bottom_right == other.bottom_right;
    }
};

// Class with methods
class Circle {
private:
    Point2D center;
    double radius;

public:
    Circle() : radius(0.0) {}
    Circle(Point2D c, double r) : center(c), radius(r) {}

    double area() const {
        return 3.14159 * radius * radius;
    }

    Point2D get_center() const { return center; }
    double get_radius() const { return radius; }

    template<typename A>
    void serialize(A& ar) {
        ar & center;
        ar & radius;
    }

    bool operator==(const Circle& other) const {
        return center == other.center && radius == other.radius;
    }
};

// Class with STL members
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;

    Person() : age(0) {}
    Person(std::string n, int a, std::vector<std::string> h)
        : name(n), age(a), hobbies(h) {}

    template<typename A>
    void serialize(A& ar) {
        ar & name;
        ar & age;
        ar & hobbies;
    }

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && hobbies == other.hobbies;
    }
};

// Asymmetric serialization with save/load
// Using a simple case: save stores computed value, load reconstructs
struct AsymmetricData {
    int value;
    int computed;  // Computed during save, not serialized directly

    AsymmetricData() : value(0), computed(0) {}
    AsymmetricData(int v) : value(v), computed(v * 2) {}

    template<typename A>
    void save(A& ar) const {
        ar & value;
        // Computed is not directly saved, recipient will recompute it
    }

    template<typename A>
    void load(A& ar) {
        ar & value;
        // Recompute the derived field
        computed = value * 2;
    }

    bool operator==(const AsymmetricData& other) const {
        return value == other.value && computed == other.computed;
    }
};

// Non-member serialization function
struct ExternalType {
    int value;
    ExternalType() : value(0) {}
    ExternalType(int v) : value(v) {}

    bool operator==(const ExternalType& other) const {
        return value == other.value;
    }
};

template<typename A>
void serialize(A& ar, ExternalType& obj) {
    ar & obj.value;
}

// Complex nested structure (defined at file level for template support)
struct ComplexShape {
    Rectangle bounds;
    Circle inscribed;
    std::vector<Point2D> vertices;

    ComplexShape() = default;

    template<typename A>
    void serialize(A& ar) {
        ar & bounds;
        ar & inscribed;
        ar & vertices;
    }

    bool operator==(const ComplexShape& other) const {
        return bounds == other.bounds &&
               inscribed == other.inscribed &&
               vertices == other.vertices;
    }
};

// Type that accesses engine during serialization
// Used to test proc_output_archive::get_engine()
struct TypeThatAccessesEngine {
    int value;

    TypeThatAccessesEngine() : value(0) {}
    TypeThatAccessesEngine(int v) : value(v) {}

    template<typename Archive>
    void save(Archive& ar) const {
        // Access engine during serialization - Covers proc_output_archive.hpp lines 15-16
        tl::engine e = ar.get_engine();
        REQUIRE(e.get_margo_instance() != MARGO_INSTANCE_NULL);
        ar & value;
    }

    template<typename Archive>
    void load(Archive& ar) {
        ar & value;
    }

    bool operator==(const TypeThatAccessesEngine& other) const {
        return value == other.value;
    }
};

TEST_SUITE("Custom Serialization") {

TEST_CASE("simple POD struct") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_point2d", [](const tl::request& req, const Point2D& p) {
        req.respond(p);
    });

    auto rpc = myEngine.define("echo_point2d");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Point2D input(3.14, 2.71);
    Point2D result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.x == 3.14);
    REQUIRE(result.y == 2.71);

    myEngine.finalize();
}

TEST_CASE("3D point struct") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_point3d", [](const tl::request& req, const Point3D& p) {
        req.respond(p);
    });

    auto rpc = myEngine.define("echo_point3d");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Point3D input(1.0, 2.0, 3.0);
    Point3D result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.x == 1.0);
    REQUIRE(result.y == 2.0);
    REQUIRE(result.z == 3.0);

    myEngine.finalize();
}

TEST_CASE("nested structures") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_rectangle", [](const tl::request& req, const Rectangle& r) {
        req.respond(r);
    });

    auto rpc = myEngine.define("echo_rectangle");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Rectangle input(Point2D(0.0, 10.0), Point2D(10.0, 0.0));
    Rectangle result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.top_left.x == 0.0);
    REQUIRE(result.top_left.y == 10.0);
    REQUIRE(result.bottom_right.x == 10.0);
    REQUIRE(result.bottom_right.y == 0.0);

    myEngine.finalize();
}

TEST_CASE("class with methods") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_circle", [](const tl::request& req, const Circle& c) {
        req.respond(c);
    });

    auto rpc = myEngine.define("echo_circle");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Circle input(Point2D(5.0, 5.0), 3.0);
    Circle result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.get_center() == Point2D(5.0, 5.0));
    REQUIRE(result.get_radius() == 3.0);

    myEngine.finalize();
}

TEST_CASE("class with STL members") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_person", [](const tl::request& req, const Person& p) {
        req.respond(p);
    });

    auto rpc = myEngine.define("echo_person");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Person input("Alice", 30, {"reading", "hiking", "coding"});
    Person result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.name == "Alice");
    REQUIRE(result.age == 30);
    REQUIRE(result.hobbies.size() == 3);
    REQUIRE(result.hobbies[0] == "reading");

    myEngine.finalize();
}

TEST_CASE("asymmetric serialization with save/load") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_asymmetric", [](const tl::request& req, const AsymmetricData& ad) {
        req.respond(ad);
    });

    auto rpc = myEngine.define("echo_asymmetric");
    tl::endpoint self_ep = myEngine.lookup(addr);

    AsymmetricData input(21);
    AsymmetricData result = rpc.on(self_ep)(input);

    // The value should match
    REQUIRE(result.value == 21);
    // The computed field should be reconstructed on load
    REQUIRE(result.computed == 42);
    REQUIRE(result == input);

    myEngine.finalize();
}

TEST_CASE("non-member serialization function") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_external", [](const tl::request& req, const ExternalType& e) {
        req.respond(e);
    });

    auto rpc = myEngine.define("echo_external");
    tl::endpoint self_ep = myEngine.lookup(addr);

    ExternalType input(42);
    ExternalType result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.value == 42);

    myEngine.finalize();
}

TEST_CASE("multiple custom types in single RPC") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("combine", [](const tl::request& req, const Point2D& p, const Circle& c) {
        // Create a circle at the point
        Circle result(p, c.get_radius());
        req.respond(result);
    });

    auto rpc = myEngine.define("combine");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Point2D point(10.0, 20.0);
    Circle circle(Point2D(0.0, 0.0), 5.0);

    Circle result = rpc.on(self_ep)(point, circle);

    REQUIRE(result.get_center() == point);
    REQUIRE(result.get_radius() == 5.0);

    myEngine.finalize();
}

TEST_CASE("default constructor requirement") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("create_default", [](const tl::request& req) {
        // Return a default-constructed object
        Point3D p;
        req.respond(p);
    });

    auto rpc = myEngine.define("create_default");
    tl::endpoint self_ep = myEngine.lookup(addr);

    Point3D result = rpc.on(self_ep)();

    REQUIRE(result.x == 0.0);
    REQUIRE(result.y == 0.0);
    REQUIRE(result.z == 0.0);

    myEngine.finalize();
}

TEST_CASE("complex nested types") {
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("echo_complex", [](const tl::request& req, const ComplexShape& cs) {
        req.respond(cs);
    });

    auto rpc = myEngine.define("echo_complex");
    tl::endpoint self_ep = myEngine.lookup(addr);

    ComplexShape input;
    input.bounds = Rectangle(Point2D(0.0, 10.0), Point2D(10.0, 0.0));
    input.inscribed = Circle(Point2D(5.0, 5.0), 5.0);
    input.vertices = {Point2D(0.0, 0.0), Point2D(10.0, 0.0), Point2D(5.0, 10.0)};

    ComplexShape result = rpc.on(self_ep)(input);

    REQUIRE(result == input);
    REQUIRE(result.vertices.size() == 3);

    myEngine.finalize();
}

TEST_CASE("serialization accesses engine from archive") {
    // Test that custom types can access engine during serialization
    // Covers proc_output_archive.hpp lines 15-16
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, true);
    std::string addr = static_cast<std::string>(myEngine.self());

    myEngine.define("serialize_with_engine", [](const tl::request& req, TypeThatAccessesEngine t) {
        req.respond(t);
    });

    auto rpc = myEngine.define("serialize_with_engine");
    tl::endpoint ep = myEngine.lookup(addr);

    TypeThatAccessesEngine input{42};
    TypeThatAccessesEngine result = rpc.on(ep)(input);

    REQUIRE(result.value == 42);

    myEngine.finalize();
}

} // TEST_SUITE
