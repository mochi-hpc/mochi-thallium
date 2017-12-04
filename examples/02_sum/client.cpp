#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup("bmi+tcp://127.0.0.1:1234");
    int ret = sum.on(server)(42,63);
    std::cout << "Server answered " << ret << std::endl;

    return 0;
}

