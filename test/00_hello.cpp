/*
 * See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

void hello(const tl::request& req, const tl::buffer& input) {
	std::cout << "(1) Hello World ";
	for(auto c : input) std::cout << c;
	std::cout << std::endl;
}

int server() {

	tl::margo_engine me("bmi+tcp://127.0.0.1:1234", MARGO_SERVER_MODE);
	me.define("hello1", hello);
	me.define("hello2", [&me](const tl::request& req, const tl::buffer& input) 
							{ std::cout << "(2) Hello World "; 
							  for(auto c : input) std::cout << c;
							  std::cout << std::endl; 
                              me.finalize(); });

	std::string addr = me.self();
	std::cout << "Server running at address " << addr << std::endl;

	return 0;
}

int client() {

	tl::margo_engine me("bmi+tcp", MARGO_CLIENT_MODE);
	auto remote_hello1 = me.define<decltype(hello)>("hello1");
	auto remote_hello2 = me.define<void(const tl::request&, const tl::buffer&)>("hello2");
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = me.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << (std::string)server_endpoint << std::endl;

	tl::buffer b(16,'a');
	
	(remote_hello1 >> server_endpoint)(b);
	
	remote_hello2.on(server_endpoint)(b);
    
	return 0;
}

int main(int argc, char** argv) {

	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0) server();
	else          client();
    std::cout << "rank " << rank << " finished its work" << std::endl;

	MPI_Finalize();
	return 0;
}
