#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure hello = myEngine.define("hello").ignore_response();
    tl::endpoint server = myEngine.lookup("bmi+tcp://127.0.0.1:1234");
    hello.on(server)();

    return 0;
}

