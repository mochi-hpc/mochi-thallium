/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_COMPLEX_SERIALIZATION_HPP
#define __THALLIUM_COMPLEX_SERIALIZATION_HPP

#include <complex>

namespace thallium {

template<class A, class T>
void save(A& ar, std::complex<T>& t) {
    ar & t.real();
    ar & t.imag();
}

template<class A, typename T>
void load(A& ar, std::complex<T>& t) {
    ar & t.real();
    ar & t.imag();
}

}

#endif
