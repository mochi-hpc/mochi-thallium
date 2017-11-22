#ifndef THALLIUM_COMPLEX_SERIALIZATION_H
#define THALLIUM_COMPLEX_SERIALIZATION_H

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
