#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << (std::string)myEngine.self() << std::endl;

    return 0;
}
