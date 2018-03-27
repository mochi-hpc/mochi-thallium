#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << (std::string)myEngine.self() << std::endl;

    std::function<void(const tl::request&, int, int)> sum = 
        [&myEngine](const tl::request& req, int x, int y) {
            std::cout << "Computing " << x << "+" << y << std::endl;
            req.respond(x+y);
            myEngine.finalize();
        };

    myEngine.define("sum", sum);

    return 0;
}
