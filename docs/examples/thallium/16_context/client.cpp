#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }

    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure process = myEngine.define("process");
    tl::endpoint server = myEngine.lookup(argv[1]);

    point q{1.0, 2.0, 3.0};
    double d = 2.0;

    point p1{4.0, 5.0, 6.0};
    point p2{7.0, 8.0, 9.0};

    // attach a serialization context to serialize the input
    auto callable = process.on(server)
                           .with_serialization_context(std::ref(q), d);
    auto response = callable(p1, p2);
    // attach a serialization context to deserialize the output
    point r = response.with_serialization_context(std::ref(q), d);

    // The above could have been written as follows:
    // point r = sum.on(server)
    //              .with_serialization_context(std::ref(q), d)
    //              (p1, p2)
    //              .with_serialization_context(std::ref(q), d);

    return 0;
}

