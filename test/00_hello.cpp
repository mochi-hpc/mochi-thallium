/*
 * See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello(const tl::request& req) {
	std::cout << "(1) Hello World" << std::endl;
}

int server() {

	tl::margo_engine me("bmi+tcp://127.0.0.1:1234", MARGO_SERVER_MODE);
	me.define("hello1", hello);
	me.define("hello2", [](const tl::request& req) { std::cout << "(2) Hello World" << std::endl; });

	std::string addr = me.self();
	std::cout << "Server running at address " << addr << std::endl;
	// TODO send address to client

	return 0;
}

int client() {

	tl::margo_engine me("bmi+tcp", MARGO_CLIENT_MODE);
	auto remote_hello1 = me.define<decltype(hello)>("hello1");
	auto remote_hello2 = me.define<void(const tl::request& req)>("hello2");
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = me.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << (std::string)server_endpoint << std::endl;
	
	(remote_hello1, server_endpoint)();
	
	remote_hello2.on(server_endpoint)();
	
	return 0;
}

int main(int argc, char** argv) {

	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0) server();
	else          client();

	MPI_Finalize();
	return 0;
}
