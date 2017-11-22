#ifndef THALLIUM_ARRAY_SERIALIZATION_H
#define THALLIUM_ARRAY_SERIALIZATION_H

#include <type_traits>
#include <array>

namespace thallium {

namespace detail {

template<class A, typename T, size_t N, bool b>
void save_array_impl(A& ar, std::array<T,N>& v, const std::integral_constant<bool, b>&) {
	for(auto& elem : v) {
		ar & elem;
	}
}

template<class A, typename T, size_t N>
void save_array_impl(A& ar, std::array<T,N>& v, const std::true_type&) {
	ar.write(&v[0],N);
}

template<class A, typename T, size_t N, bool b>
void load_array_impl(A& ar, std::array<T,N>& v, const std::integral_constant<bool, b>&) {
	for(unsigned int i=0; i<N; i++) {
		ar & v[i];
	}
}

template<class A, typename T, size_t N>
void load_array_impl(A& ar, std::array<T,N>& v, const std::true_type&) {
	ar.read(&v[0],N);
}

} // namespace detail

template<class A, typename T, size_t N>
inline void save(A& ar, std::array<T,N>& v) {
	detail::save_array_impl(ar, v, std::is_arithmetic<T>());
}

template<class A, typename T, size_t N>
inline void load(A& ar, std::array<T,N>& v) {
	detail::load_array_impl(ar, v, std::is_arithmetic<T>());
}

} // namespace thallium

#endif
