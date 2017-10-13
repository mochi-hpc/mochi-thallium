#include <thallium/serialization.hpp>

namespace thallium {

hg_return_t serialize_buffer(hg_proc_t proc, void* data) {
	std::size_t size;
	return proc::process_buffer(proc,data, size);
}

} // namespace thallium

