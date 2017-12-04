#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {

    tl::engine myEngine("bmi+tcp", THALLIUM_CLIENT_MODE);

    return 0;
}
