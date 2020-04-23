/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PROC_OBJECT_HPP
#define __THALLIUM_PROC_OBJECT_HPP

#ifdef THALLIUM_DEBUG_RPC_TYPES
#include <cxxabi.h>
#include <typeinfo>
#include <iostream>
#include <thallium/serialization/stl/string.hpp>
#endif
#include <vector>
#include <tuple>
#include <functional>
#include <mercury_proc.h>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/proc_input_archive.hpp>

namespace thallium {

#ifdef THALLIUM_DEBUG_RPC_TYPES
template<typename T>
std::string get_type_name() {
    int status;
    const char* mangled_type_name = typeid(T).name();
    char* type_name =  abi::__cxa_demangle(mangled_type_name, nullptr, nullptr, &status);
    if(status != 0) return std::string(mangled_type_name);
    std::string result;
    if(strncmp("std::tuple<", type_name, 11) == 0) {
        result = std::string(type_name+11, strlen(type_name)-12);
    } else {
        result = type_name;
    }
    free(type_name);
    return result;
}
#endif

class engine;
class request;

typedef std::function<hg_return_t(hg_proc_t)> meta_proc_fn;

inline hg_return_t meta_serialization(hg_proc_t proc, void* data) {
    auto fun = reinterpret_cast<meta_proc_fn*>(data);
    return (*fun)(proc);
}

template<typename T>
hg_return_t proc_object(hg_proc_t proc, T& data, engine* e) {
    switch (hg_proc_get_op(proc)) {
    case HG_ENCODE: {
        proc_output_archive ar(proc, *e);
#ifdef THALLIUM_DEBUG_RPC_TYPES
        std::string type_name = get_type_name<T>();
        ar << type_name;
#endif
        ar << data;
    }
    break;
    case HG_DECODE: {
        proc_input_archive ar(proc, *e);
#ifdef THALLIUM_DEBUG_RPC_TYPES
        std::string requested_type_name = get_type_name<T>();
        std::string received_type_name;
        ar >> received_type_name;
        if(requested_type_name != received_type_name) {
            std::cerr << "[THALLIUM] RPC type error: invalid decoding from "
                      << "(" << received_type_name << ") to ("
                      << requested_type_name << ")" << std::endl;
            return HG_INVALID_PARAM;
        }
#endif
        ar >> data;
    }
    break;
    case HG_FREE: {
    }
    default:
        break;
    }
    return HG_SUCCESS;
}

inline hg_return_t proc_void_object(hg_proc_t proc) {
    switch (hg_proc_get_op(proc)) {
    case HG_ENCODE: {
#ifdef THALLIUM_DEBUG_RPC_TYPES
        proc_output_archive ar(proc);
        std::string type_name = "void";
        ar << type_name;
#endif
    }
    break;
    case HG_DECODE: {
#ifdef THALLIUM_DEBUG_RPC_TYPES
        proc_input_archive ar(proc);
        std::string requested_type_name = "void";
        std::string received_type_name;
        ar >> received_type_name;
        if(requested_type_name != received_type_name) {
            std::cerr << "[THALLIUM] RPC type error: invalid decoding from "
                      << "(" << received_type_name << ") to ("
                      << requested_type_name << ")" << std::endl;
            return HG_INVALID_PARAM;
        }
#endif
    }
    break;
    case HG_FREE: {
    }
    default:
        break;
    }
    return HG_SUCCESS;
}

} // namespace thallium

#endif
