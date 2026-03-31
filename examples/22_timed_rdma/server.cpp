#include <iostream>
#include <chrono>
#include <thallium.hpp>

namespace tl = thallium;

int main() {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    // Synchronous timed pull: blocks until complete or deadline expires
    myEngine.define("timed_pull",
        [&myEngine](const tl::request& req, tl::bulk& b) {
            tl::endpoint ep = req.get_endpoint();
            std::vector<char> v(b.size());
            std::vector<std::pair<void*,std::size_t>> segments(1);
            segments[0].first  = (void*)v.data();
            segments[0].second = v.size();
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            try {
                b.on(ep).timed(std::chrono::seconds(5)) >> local;
                std::cout << "timed_pull received: ";
                for(auto c : v) std::cout << c;
                std::cout << std::endl;
                req.respond(1);
            } catch(const tl::timeout&) {
                std::cerr << "timed_pull timed out" << std::endl;
                req.respond(0);
            }
        });

    // Synchronous timed push: server sends data to the client's buffer
    myEngine.define("timed_push",
        [&myEngine](const tl::request& req, tl::bulk& b) {
            tl::endpoint ep = req.get_endpoint();
            std::string payload = "Hello!";
            std::vector<std::pair<void*,std::size_t>> segments(1);
            segments[0].first  = (void*)payload.data();
            segments[0].second = payload.size();
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::read_only);
            try {
                b.on(ep).timed(std::chrono::seconds(5)) << local;
                std::cout << "timed_push sent data" << std::endl;
                req.respond(1);
            } catch(const tl::timeout&) {
                std::cerr << "timed_push timed out" << std::endl;
                req.respond(0);
            }
        });

    // Asynchronous timed pull: initiates transfer and waits separately
    myEngine.define("async_timed_pull",
        [&myEngine](const tl::request& req, tl::bulk& b) {
            tl::endpoint ep = req.get_endpoint();
            std::vector<char> v(b.size());
            std::vector<std::pair<void*,std::size_t>> segments(1);
            segments[0].first  = (void*)v.data();
            segments[0].second = v.size();
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            try {
                tl::async_bulk_op op =
                    b.on(ep).timed(std::chrono::seconds(5)).pull_to(local);
                // ... do other work here while the transfer is in flight ...
                std::size_t transferred = op.wait();
                std::cout << "async_timed_pull received " << transferred << " bytes: ";
                for(auto c : v) std::cout << c;
                std::cout << std::endl;
                req.respond(1);
            } catch(const tl::timeout&) {
                std::cerr << "async_timed_pull timed out" << std::endl;
                req.respond(0);
            }
        });

    myEngine.wait_for_finalize();
}
