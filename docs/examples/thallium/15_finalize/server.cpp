#include <iostream>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;

class my_sum_provider : public tl::provider<my_sum_provider> {

    private:

    tl::remote_procedure m_prod;
    tl::remote_procedure m_sum;
    tl::remote_procedure m_hello;
    tl::remote_procedure m_print;

    void prod(const tl::request& req, int x, int y) {
        std::cout << "Computing " << x << "*" << y << std::endl;
        req.respond(x+y);
    }

    int sum(int x, int y) {
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

    void cleanup() {
        std::cout << "Provider with ID " << get_provider_id() <<  " is being cleaned up" << std::endl;
    }

    public:

    my_sum_provider(tl::engine& e, uint16_t provider_id=1)
    : tl::provider<my_sum_provider>(e, provider_id)
      // keep the RPCs in remote_procedure objects so we can deregister them.
    , m_prod(define("prod", &my_sum_provider::prod))
    , m_sum(define("sum", &my_sum_provider::sum))
    , m_hello(define("hello", &my_sum_provider::hello))
    , m_print(define("print", &my_sum_provider::print, tl::ignore_return_value()))
    {
        // setup a finalization callback for this provider, in case it is
        // still alive when the engine is finalized.
        get_engine().push_finalize_callback(this, [this]() { cleanup(); });
    }

    ~my_sum_provider() {
        m_prod.deregister();
        m_sum.deregister();
        m_hello.deregister();
        m_print.deregister();
        // pop the finalize callback. If this destructor was called
        // from the finalization callback, there is nothing to pop
        get_engine().pop_finalize_callback(this);
    }
};

int main(int argc, char** argv) {

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE);
    myEngine.enable_remote_shutdown();
    std::cout << "Server running at address " << myEngine.self()
        << " with provider ids 22 and 23 " << std::endl;
    // create the provider instances
    my_sum_provider myProvider22(myEngine, 22);
    my_sum_provider myProvider23(myEngine, 23);

    myEngine.wait_for_finalize();
    // the finalization callbacks will ensure that providers are freed.

    return 0;
}
