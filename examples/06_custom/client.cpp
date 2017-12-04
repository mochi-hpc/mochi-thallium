#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure dot_product = myEngine.define("dot_product");
    tl::endpoint server = myEngine.lookup("bmi+tcp://127.0.0.1:1234");
    point p(1,2,3);
    point q(5,2,4);
    double ret = dot_product.on(server)(p,q);
    std::cout << "Dot product : " << ret << std::endl;

    return 0;
}

