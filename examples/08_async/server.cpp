#include <cstdlib>
#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

tl::engine theEngine;

void sum(const tl::request& req, int x, int y) {
    std::cout << "Computing " << x << "+" << y << std::endl;
    req.respond(x+y);
}

int main() {

    theEngine = tl::engine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << theEngine.self() << std::endl;
    theEngine.define("sum", sum);

    theEngine.wait_for_finalize();

    return 0;
}

