/*
 * Copyright (c) 2020 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_FUNCTION_UTIL_HPP
#define __THALLIUM_FUNCTION_UTIL_HPP

#include <functional>
#include <type_traits>

namespace thallium {

// Removes the class component in member function types,
// e.g. remove_class<R (C::*)(A...)>::type = R(A...)
template<typename T>
struct remove_class {};

template<typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...)> {
    typedef R type(A...);
};

template<typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...) const> {
    typedef R type(A...);
};

// Extract the type of the operator() from the type of the object,
// e.g. strip_function_object<std::function<R(A...)>>::type =
//      R (std::function<R(A...)>::*operator())(A...)
template <typename F> struct strip_function_object {
    using type = typename remove_class<decltype(&F::operator())>::type;
};

// Extracts the function signature from a function, function pointer or lambda.
template <typename Function, typename F = typename std::remove_reference<Function>::type>
using function_signature = std::conditional<
    std::is_function<F>::value,
    F,
    typename std::conditional<
        std::is_pointer<F>::value || std::is_member_pointer<F>::value,
        typename std::remove_pointer<F>::type,
        typename strip_function_object<F>::type
    >::type
>;

// Checks if the provided type is std::function<...>
template<typename T>
struct is_std_function_object {
    static constexpr bool value = false;
};

template<typename R, typename ... A>
struct is_std_function_object<std::function<R(A...)>> {
    static constexpr bool value = true;
};

}

#endif
