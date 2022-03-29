#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

class my_logger : public tl::logger {

    public:

    void trace(const char* msg) const override {
        std::cout << "[trace] " << msg << std::endl;
    }

    void debug(const char* msg) const override {
        std::cout << "[debug] " << msg << std::endl;
    }

    void info(const char* msg) const override {
        std::cout << "[info] " << msg << std::endl;
    }

    void warning(const char* msg) const override {
        std::cout << "[warning] " << msg << std::endl;
    }

    void error(const char* msg) const override {
        std::cout << "[error] " << msg << std::endl;
    }

    void critical(const char* msg) const override {
        std::cout << "[critical] " << msg << std::endl;
    }
};

int main() {

    auto logger = my_logger();
    tl::logger::set_global_logger(&logger);
    tl::logger::set_global_log_level(tl::logger::level::trace);

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);

    myEngine.set_logger(&logger);
    myEngine.set_log_level(tl::logger::level::debug);

    myEngine.finalize();

    return 0;
}
