/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

void hello(const tl::request& req, const std::string& name) {
	std::cout << "Hello " << name << std::endl;
}

int server() {

	tl::engine margo("bmi+tcp://127.0.0.1:1234", MARGO_SERVER_MODE);
	margo.define("hello", hello).ignore_response();

    std::function<void(const tl::request&, int, int)> f =
        [](const tl::request& req, int x, int y) {
            std::cout << x << "+" << y << " = " << (x+y) << std::endl;
            req.respond(x+y);
        };
    margo.define("sum", f);

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
