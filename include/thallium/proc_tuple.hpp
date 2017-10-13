#ifndef __THALLIUM_PROC_TUPLE_HPP
#define __THALLIUM_PROC_TUPLE_HPP

#include <type_traits>
#include <tuple>
#include <mercury_proc.h>

namespace thallium {

namespace proc {


template<std::size_t N, typename ... T>
struct tuple_proc {

	static hg_return_t apply(hg_proc_t proc, std::tuple<T...>& t, std::size_t& size) {
		size = 0;
		std::size_t s;
		hg_return_t hret = tuple_proc<N-1>::apply(proc, t, s);
		if(hret != HG_SUCCESS) return hret;
		size += s;
		void* data = &(std::get<N-1>(t));
		hret = process_type<std::remove_reference_t<decltype(std::get<N-1>(t))>>(proc, data, s);
		if(hret != HG_SUCCESS) return hret;
		size += s;
		return HG_SUCCESS;
    }   
};  

template<typename ... T>
struct tuple_proc<0, T...> {

    static hg_return_t apply(hg_proc_t proc, std::tuple<T...>& t, std::size_t& size) {
		size = 0;
		return HG_SUCCESS;
	}
};

using namespace std;

template<template <typename ... T> class tuple, typename ... T>
hg_return_t process_type(hg_proc_t proc, void* data, std::size_t& size) {
	return tuple_proc<sizeof...(T),T...>::apply(proc, data, size);
}

} // namespace proc

} // namespace thallium

#endif
