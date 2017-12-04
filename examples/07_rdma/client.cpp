#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp", MARGO_CLIENT_MODE);
    tl::remote_procedure remote_do_rdma = myEngine.define("do_rdma").ignore_response();
    tl::endpoint server_endpoint = myEngine.lookup("bmi+tcp://127.0.0.1:1234");

    std::string buffer = "Matthieu";
    std::vector<std::pair<void*,std::size_t>> segments(1);
    segments[0].first  = (void*)(&buffer[0]);
    segments[0].second = buffer.size()+1;

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    remote_do_rdma.on(server_endpoint)(myBulk);

    return 0;
}
