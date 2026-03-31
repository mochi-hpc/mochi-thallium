#include <iostream>
#include <chrono>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp://127.0.0.1:1234", THALLIUM_SERVER_MODE);

    std::function<void(const tl::request&, tl::bulk&)> f =
        [&myEngine](const tl::request& req, tl::bulk& b) {
            tl::endpoint ep = req.get_endpoint();
            std::vector<char> v(6);
            std::vector<std::pair<void*,std::size_t>> segments(1);
            segments[0].first  = (void*)(&v[0]);
            segments[0].second = v.size();
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            try {
                b.on(ep).timed(std::chrono::seconds(5)) >> local;
                std::cout << "Received: ";
                for(auto c : v) std::cout << c;
                std::cout << std::endl;
            } catch(const tl::timeout&) {
                std::cout << "Transfer timed out!" << std::endl;
            }
            req.respond();
        };
    myEngine.define("do_timed_rdma", f);

    myEngine.wait_for_finalize();
}
