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
    tl::buffer ret(6,'c');
    req.respond(ret);
}

int server() {

	tl::engine margo("bmi+tcp://127.0.0.1:1234", MARGO_SERVER_MODE);
	margo.define("hello1", hello);
	margo.define("hello2", [&margo](const tl::request& req, const tl::buffer& input) 
							{ std::cout << "(2) Hello World "; 
							  for(auto c : input) std::cout << c;
							  std::cout << std::endl; 
                              margo.finalize(); }
                        ).ignore_response();
	std::string addr = margo.self();
	std::cout << "Server running at address " << addr << std::endl;

	return 0;
}

int client() {

	tl::engine margo("bmi+tcp", MARGO_CLIENT_MODE);
	auto remote_hello1 = margo.define("hello1");
	auto remote_hello2 = margo.define("hello2").ignore_response();
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = margo.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << (std::string)server_endpoint << std::endl;

	tl::buffer b(16,'a');
	
	auto ret = remote_hello1.on(server_endpoint)(b);
    std::cout << "Response from hello1: ";
    for(auto c : ret) std::cout << c;
    std::cout << std::endl;
	
	remote_hello2.on(server_endpoint)(b);
    
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
