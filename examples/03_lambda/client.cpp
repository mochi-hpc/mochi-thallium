#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = myEngine.define("sum");
    tl::remote_procedure mult = myEngine.define("mult");
    tl::endpoint server = myEngine.lookup(argv[1]);
    int ret = sum.on(server)(42,63);
    std::cout << "Server answered (sum)" << ret << std::endl;
    ret = mult.on(server)(42,63);
    std::cout << "Server answered (mult)" << ret << std::endl;

    return 0;
}

