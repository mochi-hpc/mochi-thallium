#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, int, int)> sum = 
        [&myEngine](const tl::request& req, int x, int y) {
            std::cout << "Computing " << x << "+" << y << std::endl;
            req.respond(x+y);
            myEngine.finalize();
        };

    myEngine.define("sum", sum);

    myEngine.push_finalize_callback([]() { std::cout << "Finalization was called" << std::endl; });

    myEngine.wait_for_finalize();

    return 0;
}

