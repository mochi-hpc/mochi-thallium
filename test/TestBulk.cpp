/*
 * See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <unistd.h>
#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

int server() {

	tl::engine margo("bmi+tcp://127.0.0.1:1234", MARGO_SERVER_MODE);

    std::function<void(const tl::request&, tl::bulk& b)> f =
        [&margo](const tl::request& req, tl::bulk& b) {
            auto ep = req.get_endpoint();
            std::vector<char> v(6,'*');
            std::vector<std::pair<void*,std::size_t>> seg(1);
            seg[0].first  = (void*)(&v[0]);
            seg[0].second = v.size();
            tl::bulk local = margo.expose(seg, tl::bulk_mode::write_only);
            b(1,5).on(ep) >> local;
            std::cout << "Server received bulk: ";
            for(auto c : v) std::cout << c;
            std::cout << std::endl;
        };
	margo.define("send_bulk",f).ignore_response();

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
	auto remote_send = margo.define("send_bulk").ignore_response();
    auto remote_stop = margo.define("stop").ignore_response();
	std::string server_addr = "bmi+tcp://127.0.0.1:1234";
	sleep(1);

	auto server_endpoint = margo.lookup(server_addr);
	std::cout << "Lookup done for endpoint " << (std::string)server_endpoint << std::endl;

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

	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0) server();
	else          client();
    std::cout << "Rank " << rank << " finished its work" << std::endl;

	MPI_Finalize();
	return 0;
}
