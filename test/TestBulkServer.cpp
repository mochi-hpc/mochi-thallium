/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>
#ifdef USE_CEREAL
#include <cereal/types/string.hpp>
#else
#include <thallium/serialization/stl/string.hpp>
#endif

namespace tl = thallium;

int server() {

    tl::engine margo("tcp://127.0.0.1:1234", MARGO_SERVER_MODE);

    std::function<void(const tl::request&, tl::bulk& b)> f =
        [&margo](const tl::request& req, tl::bulk& b) {
            auto ep = req.get_endpoint();
            std::vector<char> v(6,'*');
            std::vector<std::pair<void*,std::size_t>> seg(1);
            seg[0].first  = (void*)(&v[0]);
            seg[0].second = v.size();
            tl::bulk local = margo.expose(seg, tl::bulk_mode::write_only);
            b.on(ep) >> local;
            std::cout << "Server received bulk: ";
            for(auto c : v) std::cout << c;
            std::cout << std::endl;
        };
    margo.define("send_bulk",f).disable_response();

    std::function<void(const tl::request&)> g =
        [&margo](const tl::request& req) {
            std::cout << "Stopping server" << std::endl;
            margo.finalize();
        };
    margo.define("stop", g);

    std::string addr = margo.self();
    std::cout << "Server running at address " << addr << std::endl;

    return 0;
}

int main(int argc, char** argv) {
	server();
	return 0;
}
