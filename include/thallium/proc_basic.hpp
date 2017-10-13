#ifndef __THALLIUM_PROC_BASIC_HPP
#define __THALLIUM_PROC_BASIC_HPP

#include <type_traits>
#include <mercury_proc.h>

namespace thallium {

namespace proc {

template<typename T, 
	typename std::enable_if_t<
		std::is_arithmetic<
			std::remove_reference_t<T>
		>::value> = 0>
hg_return_t process_type(hg_proc_t proc, void *data, std::size_t& size) {
	T* t = static_cast<T*>(data);
	size = sizeof(*t);
	return hg_proc_memcpy(proc, data, sizeof(*t));
}

} // namespace proc

} // namespace thallium

#endif
