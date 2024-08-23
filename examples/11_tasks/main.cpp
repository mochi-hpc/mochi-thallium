#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello() {
    tl::xstream es = tl::xstream::self();
    std::cout << "Hello World from ES "
        << es.get_rank() << ", TASK "
        << tl::task::self_id() << std::endl;
}

int main() {

    tl::abt scope;

    std::vector<tl::managed<tl::xstream>> ess;

    tl::xstream primary = tl::xstream::self();
    (void)primary;

    for(int i=0; i < 4; i++) {
        tl::managed<tl::xstream> es = tl::xstream::create();
        ess.push_back(std::move(es));
    }

    std::vector<tl::managed<tl::task>> tsks;
    for(int i=0; i < 16; i++) {
        tl::managed<tl::task> tsk = ess[i % ess.size()]->make_task(hello);
        tsks.push_back(std::move(tsk));
    }

    for(auto& mtsk : tsks) {
        mtsk->join();
    }

    for(int i=0; i < 4; i++) {
        ess[i]->join();
    }

    return 0;
}
