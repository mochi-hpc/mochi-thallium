#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure dot_product = myEngine.define("dot_product");
    tl::endpoint server = myEngine.lookup(argv[1]);
    point p(1,2,3);
    point q(5,2,4);
    double ret = dot_product.on(server).async(p,q).wait();
    std::cout << "Dot product : " << ret << std::endl;

    return 0;
}

