/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REF_UTIL_HPP
#define __THALLIUM_REF_UTIL_HPP

#include <type_traits>

// the code bellow is taken from
// https://en.cppreference.com/w/cpp/utility/functional/unwrap_reference

namespace thallium {

template <class T>
struct unwrap_refwrapper
{
    using type = T;
};

template <class T>
struct unwrap_refwrapper<std::reference_wrapper<T>>
{
    using type = T&;
};

template <class T>
using unwrap_decay_t = typename unwrap_refwrapper<typename std::decay<T>::type>::type;

}

#endif
