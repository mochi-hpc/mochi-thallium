/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_FUNCTION_CAST_HPP
#define __THALLIUM_FUNCTION_CAST_HPP

#include <cstdint>

namespace thallium {

/**
 * @brief Cast a void* into a function type.
 *
 * @tparam F Function type.
 * @param f pointer to convert.
 *
 * @return Function pointer.
 */
template <typename F> F* function_cast(void* f) {
    return reinterpret_cast<F*>(reinterpret_cast<std::intptr_t>(f));
}

/**
 * @brief Cast a function type into a void* pointer.
 *
 * @tparam F Function type.
 * @param fun pointer to convert.
 *
 * @return A void* pointer to the function.
 */
template <typename F> void* void_cast(F&& fun) {
    return reinterpret_cast<void*>(reinterpret_cast<std::intptr_t>(fun));
}

} // namespace thallium

#endif
