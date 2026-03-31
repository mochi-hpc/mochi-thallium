#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
        exit(0);
    }

    tl::engine myEngine("tcp", MARGO_CLIENT_MODE);
    tl::endpoint server = myEngine.lookup(argv[1]);

    // Demonstrate timed_pull: client exposes a read-only buffer for the server to pull
    {
        std::string data = "Matthieu";
        std::vector<std::pair<void*,std::size_t>> segments(1);
        segments[0].first  = (void*)data.data();
        segments[0].second = data.size();
        tl::bulk b = myEngine.expose(segments, tl::bulk_mode::read_only);
        int ok = myEngine.define("timed_pull").on(server)(b);
        std::cout << "timed_pull: " << (ok ? "ok" : "timed out") << std::endl;
    }

    // Demonstrate timed_push: client exposes a write-only buffer for the server to push into
    {
        std::vector<char> recv(6, '\0');
        std::vector<std::pair<void*,std::size_t>> segments(1);
        segments[0].first  = (void*)recv.data();
        segments[0].second = recv.size();
        tl::bulk b = myEngine.expose(segments, tl::bulk_mode::write_only);
        int ok = myEngine.define("timed_push").on(server)(b);
        std::cout << "timed_push: " << (ok ? "ok" : "timed out") << std::endl;
        if(ok) {
            std::cout << "received: ";
            for(auto c : recv) std::cout << c;
            std::cout << std::endl;
        }
    }

    // Demonstrate async_timed_pull
    {
        std::string data = "Thallium";
        std::vector<std::pair<void*,std::size_t>> segments(1);
        segments[0].first  = (void*)data.data();
        segments[0].second = data.size();
        tl::bulk b = myEngine.expose(segments, tl::bulk_mode::read_only);
        int ok = myEngine.define("async_timed_pull").on(server)(b);
        std::cout << "async_timed_pull: " << (ok ? "ok" : "timed out") << std::endl;
    }

    return 0;
}
