#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&)> process =
        [](const tl::request& req) {
            point p1, p2;
            point q{3.0,2.0,1.0};
            double d = 3.5;
            std::tie(p1, p2) =
                req.get_input()
                   .with_serialization_context(std::ref(q), d)
                   .as<point,point>();
            std::cout << "Executing RPC" << std::endl;
            req.with_serialization_context(std::ref(q), d)
               .respond(p1+p2);
        };

    myEngine.define("process", process);

    myEngine.wait_for_finalize();

    return 0;
}

