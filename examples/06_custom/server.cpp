#include <iostream>
#include <thallium.hpp>
#include "point.hpp"

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, const point&, const point&)> dot_product =
        [&myEngine](const tl::request& req, const point& p, const point& q) {
            point pq;
            tl::auto_respond<point> response{req, pq};
            pq = p*q;
            myEngine.finalize();
        };

    myEngine.define("dot_product", dot_product);

    myEngine.wait_for_finalize();

    return 0;
}

