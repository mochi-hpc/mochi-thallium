#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, int, int)> sum = 
        [](const tl::request& req, int x, int y) {
            std::cout << "Computing " << x << "+" << y << std::endl;
            req.respond(x+y);
        };

    myEngine.define("sum", sum);

    myEngine.define("mult", [](const tl::request& req, int x, int y) {
         std::cout << "Computing " << x << "*" << y << std::endl;
         req.respond(x*y);
    });

    return 0;
}

