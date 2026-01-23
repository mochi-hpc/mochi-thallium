#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello(const tl::request& req) {
    std::cout << "Hello World!" << std::endl;
}

int main(int argc, char** argv) {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    myEngine.define("hello", hello).disable_response();
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    myEngine.wait_for_finalize();

    return 0;
}
