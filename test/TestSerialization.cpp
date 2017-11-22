#include <cassert>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/tuple.hpp>

using namespace thallium;

void SerializeValues() {
    buffer buf;
	
    double a1	= 1.0;
    int    b1	= 42;
    char   c1	= 'c';

    {
        buffer_output_archive arch(buf);
        arch & a1 & b1 & c1;
    }

    double a2;
    int    b2;
    char   c2;

    {
        buffer_input_archive arch(buf);
        arch & a2 & b2 & c2;
    }

    assert(a1 == a2);
    assert(b1 == b2);
    assert(c1 == c2);
};

int main(int argc, char** argv) {
    SerializeValues();
	return 0;
}
