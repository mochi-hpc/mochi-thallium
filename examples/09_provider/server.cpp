#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

class my_sum_provider : public tl::provider<my_sum_provider> {

    private:

    void prod(const tl::request& req, int x, int y) {
        std::cout << "Computing " << x << "*" << y << std::endl;
        req.respond(x*y);
    }

    int sum(int x, int y) const {
        std::cout << "Computing " << x << "+" << y << std::endl;
        return x+y;
    }

    void hello(const std::string& name) {
        std::cout << "Hello, " << name << std::endl;
    }

    int print(const std::string& word) {
        std::cout << "Printing " << word << std::endl;
        return word.size();
    }

    public:

    my_sum_provider(const tl::engine& e, uint16_t provider_id=1)
    : tl::provider<my_sum_provider>(e, provider_id) {
        define("prod", &my_sum_provider::prod);
        define("sum", &my_sum_provider::sum);
        define("hello", &my_sum_provider::hello).disable_response();
        define("print", &my_sum_provider::print, tl::ignore_return_value());
    }

    ~my_sum_provider() {
        get_engine().wait_for_finalize();
    }
};

int main() {

    uint16_t provider_id = 22;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self()
        << " with provider id " << provider_id << std::endl;
    my_sum_provider myProvider(myEngine, provider_id);

    return 0;
}

