#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(argv[1]);

    auto response = sum.on(server).async(42,63);
    int ret = response.wait();
    std::cout << "Server answered " << ret << std::endl;

    std::vector<tl::async_response> reqs;
    for(unsigned i=0; i < 10; i++) {
        auto response = sum.on(server).async(42,63);
        reqs.push_back(std::move(response));
    }
    for(auto i=0; i < 10; i++) {
        int ret;
        decltype(reqs.begin()) completed;
        ret = tl::async_response::wait_any(reqs.begin(), reqs.end(), completed);
        reqs.erase(completed);
        std::cout << "Server answered " << ret << std::endl;
    }

    return 0;
}

