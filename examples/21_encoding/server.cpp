#include <iostream>
#include <thallium.hpp>
#include "encoder.hpp"

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, const encoder&)> stream =
        [&myEngine](const tl::request& req, const encoder&) {
            req.respond();
        };

    myEngine.define("stream", stream);
    myEngine.wait_for_finalize();

    return 0;
}

