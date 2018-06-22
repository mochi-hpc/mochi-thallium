#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello() {
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES " 
        << es.get_rank() << ", ULT " 
        << tl::thread::self_id() << std::endl;
}

int main(int argc, char** argv) {

    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);

    std::vector<tl::managed<tl::xstream>> ess;

    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es = tl::xstream::create();
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
