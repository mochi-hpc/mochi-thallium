#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

class my_sum_provider : public tl::provider<my_sum_provider> {

    private:

    tl::auto_remote_procedure m_prod_rpc;
    tl::auto_remote_procedure m_sum_rpc;
    tl::auto_remote_procedure m_hello_rpc;
    tl::auto_remote_procedure m_print_rpc;

    void prod(const tl::request& req, int x, int y) {
        std::cout << "Computing " << x << "*" << y << std::endl;
        req.respond(x*y);
    }

    int sum(int x, int y) const {
        std::cout << "Computing " << x << "+" << y << std::endl;
        return x+y;
    }

    void hello(const std::string& name) {
        std::cout << "Hello, " << name << ", from " << identity() << std::endl;
    }

    int print(const std::string& word) {
        std::cout << "Printing " << word << std::endl;
        return word.size();
    }

    public:

    my_sum_provider(const tl::engine& e, uint16_t provider_id)
    : tl::provider<my_sum_provider>(e, provider_id, "myprovider")
    , m_prod_rpc{define("prod", &my_sum_provider::prod)}
    , m_sum_rpc{define("sum", &my_sum_provider::sum)}
    , m_hello_rpc{define("hello", &my_sum_provider::hello).disable_response()}
    , m_print_rpc{define("print", &my_sum_provider::print, tl::ignore_return_value())}
    {}
};

int main() {

    uint16_t provider_id = 22;
    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self()
              << " with provider id " << provider_id << std::endl;

    auto provider = new my_sum_provider(myEngine, provider_id);
    myEngine.push_finalize_callback(provider, [provider]() { delete provider; });

    myEngine.wait_for_finalize();

    return 0;
}

