#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);

    {
        // if we don't put all this in a scope, the timed_callback
        // will continue living after finalize, causing a segfault
        auto timed_cb = myEngine.create_timed_callback(
            []() { std::cout << "Calling the timed callback" << std::endl; });

        std::cout << "Starting the timed_callback" << std::endl;
        timed_cb.start(1000);
        tl::thread::sleep(myEngine, 500);
        std::cout << "This should be before the callback" << std::endl;
        tl::thread::sleep(myEngine, 700);
        std::cout << "This should be after the callback" << std::endl;
        std::cout << "Restarting the callback" << std::endl;
        timed_cb.start(1000);
        tl::thread::sleep(myEngine, 500);
        std::cout << "Cancelling the callback" << std::endl;
        timed_cb.cancel();
    }

    myEngine.finalize();

    return 0;
}
