#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello() {
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES "
        << es.get_rank() << ", ULT "
        << tl::thread::self_id() << std::endl;
}

int main() {

    tl::abt scope;

    std::vector<tl::managed<tl::xstream>> ess;

    tl::managed<tl::pool> myPool = tl::pool::create(tl::pool::access::spmc);

    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es
            = tl::xstream::create(tl::scheduler::predef::deflt, *myPool);
        ess.push_back(std::move(es));
    }

    std::vector<tl::managed<tl::thread>> ths;
    for(int i=0; i < 16; i++) {
        tl::managed<tl::thread> th = ess[i % ess.size()]->make_thread(hello);
        ths.push_back(std::move(th));
    }

    for(auto& mth : ths) {
        mth->join();
    }

    for(int i=0; i < 4; i++) {
        ess[i]->join();
    }

    return 0;
}
