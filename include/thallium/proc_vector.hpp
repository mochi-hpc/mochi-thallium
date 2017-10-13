#ifndef __THALLIUM_PROC_VECTOR_HPP
#define __THALLIUM_PROC_VECTOR_HPP

#include <vector>
#include <mercury_proc.h>

namespace thallium {

namespace proc {

using namespace std;

template<template <typename T> class vector, class T,
		typename std::enable_if_t<
			std::is_arithmetic<
				std::remove_reference_t<T>
			>::value
		> = 0>
hg_return_t process_type(hg_proc_t proc, void* data, std::size_t& size) {
	std::vector<T> *vec = static_cast<std::vector<T>*>(data);
	std::size_t num_T = 0; 
	hg_return_t hret;
	size = 0;
	switch(hg_proc_get_op(proc)) {
		case HG_ENCODE:
			num_T = vec->size();
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			if (num_T > 0)
				hret = hg_proc_memcpy(proc, vec->data(), num_T*sizeof(T));
			if (hret != HG_SUCCESS) return hret;
			size += num_T*sizeof(T);
            break;
		case HG_DECODE:
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			if (num_T > 0) {
				vec->resize(num_T);
				hret = hg_proc_memcpy(proc, vec->data(), num_T*sizeof(T));
				if (hret != HG_SUCCESS) return hret;
				size += num_T*sizeof(T);
			}
			break;
		case HG_FREE:
			return HG_SUCCESS;
	} 
	return HG_SUCCESS;
}

template<template <typename T> class vector, class T>
hg_return_t process_type(hg_proc_t proc, void* data, std::size_t& size) {
	std::vector<T> *vec = static_cast<std::vector<T>*>(data);
	std::size_t num_T = 0; 
	hg_return_t hret;
	size = 0;
	switch(hg_proc_get_op(proc)) {
		case HG_ENCODE:
			num_T = vec->size();
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			char* ptr = vec->data();
			for(auto i=0; i < num_T; i++) {
				std::size_t s;
				hret = process_type<T>(proc, static_cast<void*>(ptr), s);
				if (hret != HG_SUCCESS) return hret;
				ptr += s;
				size += s;
			}
            break;
		case HG_DECODE:
			hret = hg_proc_memcpy(proc, &num_T, sizeof(num_T));
			if (hret != HG_SUCCESS) return hret;
			size += sizeof(num_T);
			if (num_T > 0) {
				vec->resize(num_T);
				char* ptr = vec->data();
				for(auto i=0; i < num_T; i++) {
					std::size_t s;
					hret = process_type<T>(proc, static_cast<void*>(ptr), s);
					if (hret != HG_SUCCESS) return hret;
					ptr += s;
					size += s;
				}
			}
			break;
		case HG_FREE:
			return HG_SUCCESS;
	} 
	return HG_SUCCESS;
}

} // namespace proc

} // namespace thallium

#endif
