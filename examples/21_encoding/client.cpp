#include <iostream>
#include <thallium.hpp>
#include "encoder.hpp"

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure stream = myEngine.define("stream");
    tl::endpoint server = myEngine.lookup(argv[1]);
    encoder e;
    stream.on(server)(e);

    return 0;
}

