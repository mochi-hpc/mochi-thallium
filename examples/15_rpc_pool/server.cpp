#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void sum(const tl::request& req, int x, int y) {
    std::cout << "Computing " << x << "+" << y << std::endl;
    req.respond(x+y);
}

int main() {

    tl::abt scope;

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);

    std::vector<tl::managed<tl::xstream>> ess;
    tl::managed<tl::pool> myPool = tl::pool::create(tl::pool::access::spmc);
    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es
            = tl::xstream::create(tl::scheduler::predef::deflt, *myPool);
        ess.push_back(std::move(es));
    }

    std::cout << "Server running at address " << myEngine.self() << std::endl;
    myEngine.define("sum", sum, 1, *myPool);

    myEngine.wait_for_finalize();

    for(int i=0; i < 4; i++) {
        ess[i]->join();
    }

    return 0;
}

