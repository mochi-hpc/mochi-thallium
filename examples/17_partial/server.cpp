#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

/* IMPORTANT: this code will only run correctly
 * if the "checksum" variant was disabled in
 * Mercury
 */

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, int)> sum =
        [](const tl::request& req, int x) {
            std::cout << "Starting RPC" << std::endl;
            int y;
            std::tie(x, y) = req.get_input().as<int,int>();
            std::cout << "x = " << x << ", y = " << y << std::endl;
            std::cout << "Computing " << x << "+" << y << std::endl;
            req.respond(x+y);
        };

    myEngine.define("sum", sum);

    myEngine.wait_for_finalize();
    return 0;
}

