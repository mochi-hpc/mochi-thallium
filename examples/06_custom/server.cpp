#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp://127.0.0.1:1234", THALLIUM_SERVER_MODE);

    std::function<void(const tl::request&, const point&, const point&)> dot_product = 
        [&myEngine](const tl::request& req, const point& p, const point& q) {
            req.respond(p*q);
            myEngine.finalize();
        };

    myEngine.define("dot_product", dot_product);

    return 0;
}

