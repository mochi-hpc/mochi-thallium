#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    const char* config = R"(
    {
      "argobots": {
        "pools": [
          {
            "name": "__primary__",
            "kind": "fifo_wait",
            "access": "mpmc"
          },
          {
            "name": "my_pool",
            "kind": "prio_wait",
            "access": "mpmc"
          },
          {
            "name": "my_other_pool",
            "kind": "fifo",
            "access": "mpmc"
          }
        ],
        "xstreams": [
          {
            "name": "__primary__",
            "scheduler": {
              "type": "basic_wait",
              "pools": [0]
            }
          },
          {
            "name": "my_es",
            "scheduler": {
              "type": "basic_wait",
              "pools": ["my_pool", 2]
            }
          }
        ]
      }
    }
    )";

    tl::engine myEngine("tcp", THALLIUM_SERVER_MODE, config);

    std::cout << myEngine.get_config() << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << myEngine.pools().size() << " pools, ";
    std::cout << myEngine.xstreams().size() << " xstreams" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    tl::pool myPool = myEngine.pools()["my_pool"];
    tl::pool myOtherPool = myEngine.pools()[2];
    std::cout << myEngine.pools()[1].name() << std::endl;

    tl::xstream myES = myEngine.xstreams()["my_es"];
    tl::xstream primaryES = myEngine.xstreams()[0];
    std::cout << myEngine.xstreams()[1].name() << std::endl;

    myEngine.finalize();

    return 0;
}
