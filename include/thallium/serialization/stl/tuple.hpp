/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_TUPLE_SERIALIZATION_HPP
#define __THALLIUM_TUPLE_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/tuple.hpp>
#else

#include <tuple>

namespace thallium {

namespace detail {

template<size_t N>
struct tuple_serializer {

    template<class A, class... Types>
    static inline void apply(A& ar, std::tuple<Types...>& t) {
        tuple_serializer<N-1>::apply(ar,t);
        ar & std::get<N-1>(t);
    }
};

template<>
struct tuple_serializer<0> {

    template<class A, class... Types>
    static inline void apply(A& ar, std::tuple<Types...>& t) {
        (void)ar;
        (void)t;
    }
};

}

template<class A, class... Types>
inline void save(A& ar, std::tuple<Types...>& t) {
    detail::tuple_serializer<sizeof...(Types)>::apply(ar,t);
}

template<class A, typename... Types>
inline void load(A& ar, std::tuple<Types...>& t) {
    detail::tuple_serializer<sizeof...(Types)>::apply(ar,t);
}

}

#endif
#endif
