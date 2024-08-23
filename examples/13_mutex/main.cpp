#include <iostream>
#include <unistd.h>
#include <thallium.hpp>

namespace tl = thallium;

int myCounter = 0;

void hello(tl::mutex& mtx) {
    tl::xstream es = tl::xstream::self();
    mtx.lock();
    std::cout << "Hello World from ES "
        << es.get_rank() << ", ULT "
        << tl::thread::self_id()
        << ", counter = " << myCounter << std::endl;
    myCounter += 1;
    mtx.unlock();
}

int main() {

    tl::abt scope;

    std::vector<tl::managed<tl::xstream>> ess;

    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es = tl::xstream::create();
        ess.push_back(std::move(es));
    }

    tl::mutex myMutex;

    std::vector<tl::managed<tl::thread>> ths;
    for(int i=0; i < 16; i++) {
        tl::managed<tl::thread> th
            = ess[i % ess.size()]->make_thread([&myMutex]() {
                    hello(myMutex);
        });
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
