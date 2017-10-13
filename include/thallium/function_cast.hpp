#ifndef __THALLIUM_FUNCTION_CAST_HPP
#define __THALLIUM_FUNCTION_CAST_HPP

#include <cstdint>

namespace thallium {

template<typename F>
F* function_cast(void* f) {
	return reinterpret_cast<F*>(reinterpret_cast<std::intptr_t>(f));
}

template<typename F>
void* void_cast(F&& fun) {
	return reinterpret_cast<void*>(reinterpret_cast<std::intptr_t>(fun));
}

}

#endif
