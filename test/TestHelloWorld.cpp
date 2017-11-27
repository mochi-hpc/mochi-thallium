/*
 * See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
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

	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0) server();
	else          client();
    std::cout << "Rank " << rank << " finished its work" << std::endl;

	MPI_Finalize();
	return 0;
}
