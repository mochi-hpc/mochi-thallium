#ifndef __THALLIUM_SERIALIZATION_HPP
#define __THALLIUM_SERIALIZATION_HPP

#include <cstdint>
#include <mercury_proc.h>

namespace thallium {
namespace proc {

template<typename T> hg_return_t process_type(hg_proc_t proc, void* data, std::size_t& size);

}
}

#include <thallium/buffer.hpp>
#include <thallium/proc_buffer.hpp>
//#include <thallium/proc_basic.hpp>
//#include <thallium/proc_tuple.hpp>
//#include <thallium/proc_vector.hpp>

namespace thallium {

/*
template<typename T>
hg_return_t serialize(hg_proc_t proc, void* data) {
	std::size_t size;
	return proc::process_type<T>(proc, data, size);
}
*/
template<typename ... T>
hg_return_t serialize(hg_proc_t proc, void* data) {
	std::size_t size;
	return proc::process_type<std::tuple<T...>>(proc, data, size);
}

hg_return_t serialize_buffer(hg_proc_t proc, void* data);

} // namespace thallium

#endif
