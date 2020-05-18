/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_TUPLE_UTIL_HPP
#define __THALLIUM_TUPLE_UTIL_HPP

#include <tuple>

namespace thallium {

namespace detail {

template <size_t N> struct apply_f_to_t_impl {
    template <typename R, typename... ArgsF, typename... ArgsT,
              typename... Args>
    static R apply(const std::function<R(ArgsF...)>& f, std::tuple<ArgsT...>& t,
                   Args&&... args) {
        return apply_f_to_t_impl<N - 1>::apply(f, t, std::get<N - 1>(t),
                                               std::forward<Args>(args)...);
    }
};

template <> struct apply_f_to_t_impl<0> {
    template <typename R, typename... ArgsF, typename... ArgsT,
              typename... Args>
    static R apply(const std::function<R(ArgsF...)>& f, std::tuple<ArgsT...>& t,
                   Args&&... args) {
        (void)t;
        return f(std::forward<Args>(args)...);
    }
};

} // namespace detail

/**
 * Applies a function with arbitrary arguments to a tuple holding the same types
 * of arguments.
 *
 * \param f : function to call on the tuple.
 * \param t : tupe of arguments.
 * \return the value returned by f.
 */
template <typename R, typename... ArgsF, typename... ArgsT>
R apply_function_to_tuple(const std::function<R(ArgsF...)>& f,
                          std::tuple<ArgsT...>&             t) {
    return detail::apply_f_to_t_impl<sizeof...(ArgsT)>::apply(f, t);
}

/**
 * Applies a function with arbitrary arguments to a tuple holding the same types
 * of arguments.
 *
 * \param f : function to call on the tuple.
 * \param t : tupe of arguments.
 * \return the value returned by f.
 */
template <typename R, typename... ArgsF, typename... ArgsT>
R apply_function_to_tuple(R (*f)(ArgsF...), std::tuple<ArgsT...>& t) {
    std::function<R(ArgsF...)> fun(f);
    return apply_function_to_tuple(fun, t);
}

} // namespace thallium

#endif
