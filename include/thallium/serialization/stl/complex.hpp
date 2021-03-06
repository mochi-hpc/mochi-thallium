/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_COMPLEX_SERIALIZATION_HPP
#define __THALLIUM_COMPLEX_SERIALIZATION_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL
    #include <cereal/types/complex.hpp>
#else

#include <complex>

namespace thallium {

template<class A, class T>
inline void save(A& ar, std::complex<T>& t) {
    ar & t.real();
    ar & t.imag();
}

template<class A, typename T>
inline void load(A& ar, std::complex<T>& t) {
    ar & t.real();
    ar & t.imag();
}

}

#endif
#endif
