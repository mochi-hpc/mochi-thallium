/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_VECTOR_SERIALIZATION_HPP
#define __THALLIUM_VECTOR_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/vector.hpp>
#else

#include <type_traits>
#include <vector>

namespace thallium {

namespace detail {

template<class A, typename T, class Alloc, bool b1, bool b2>
inline void save_vector_impl(A& ar, std::vector<T,Alloc>& v,
        const std::integral_constant<bool, b1>&,
        const std::integral_constant<bool, b2>&) {
    size_t size = v.size();
    ar.write(&size);
    for(auto& elem : v) {
        ar & elem;
    }
}

template<class A, typename T, class Alloc>
inline void save_vector_impl(A& ar, std::vector<T,Alloc>& v,
        const std::true_type&,
        const std::false_type&) {
    size_t size = v.size();
    ar.write(&size);
    ar.write(v.data(), size);
}

template<class A, typename T, class Alloc, bool b1, bool b2>
inline void load_vector_impl(A& ar, std::vector<T,Alloc>& v,
        const std::integral_constant<bool, b1>&,
        const std::integral_constant<bool, b2>&) {
    size_t size;
    ar.read(&size);
    v.clear();
    v.resize(size);
    for(unsigned int i=0; i<size; i++) {
        ar & v[i];
    }
}

template<class A, typename T, class Alloc>
inline void load_vector_impl(A& ar, std::vector<T,Alloc>& v,
        const std::true_type&,
        const std::false_type&) {
    size_t size;
    ar.read(&size);
    v.clear();
    v.resize(size);
    ar.read(v.data(), size);
}

} // namespace detail

template<class A, typename T, class Alloc>
inline void save(A& ar, std::vector<T,Alloc>& v) {
    detail::save_vector_impl(ar, v, std::is_arithmetic<T>(), std::is_same<T, bool>());
}

template<class A, typename T, class Alloc>
inline void load(A& ar, std::vector<T,Alloc>& v) {
    detail::load_vector_impl(ar, v, std::is_arithmetic<T>(), std::is_same<T, bool>());
}

} // namespace thallium

#endif
#endif
