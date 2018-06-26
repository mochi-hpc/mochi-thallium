/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

int client() {

	tl::engine margo("bmi+tcp", MARGO_CLIENT_MODE);
	auto remote_send = margo.define("send_bulk").disable_response();
    auto remote_stop = margo.define("stop").disable_response();
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = margo.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << server_endpoint << std::endl;

    std::string buf = "Matthieu";
    std::vector<std::pair<void*,std::size_t>> seg(1);
    seg[0].first  = (void*)(&buf[0]);
    seg[0].second = buf.size()+1;

    tl::bulk b = margo.expose(seg, tl::bulk_mode::read_only);

    remote_send.on(server_endpoint)(b);
	
    remote_stop.on(server_endpoint)();
    
	return 0;
}

int main(int argc, char** argv) {
	client();
	return 0;
}
