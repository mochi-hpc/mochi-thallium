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
	auto remote_hello = margo.define("hello").ignore_response();
	auto remote_sum   = margo.define("sum");
    auto remote_stop  = margo.define("stop").ignore_response();
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = margo.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << (std::string)server_endpoint << std::endl;

	remote_hello.on(server_endpoint)(std::string("Matt"));
	
    int ret = remote_sum.on(server_endpoint)(23,67);

    std::cout << "Server returned " << ret << std::endl;

    remote_stop.on(server_endpoint)();
    
	return 0;
}

int main(int argc, char** argv) {
	client();
	return 0;
}
