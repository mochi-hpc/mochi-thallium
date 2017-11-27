/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PACKED_RESPONSE_HPP
#define __THALLIUM_PACKED_RESPONSE_HPP

#include <thallium/buffer.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>

namespace thallium {

class callable_remote_procedure;

class packed_response {

    friend class callable_remote_procedure;

private:

    buffer m_buffer;

    packed_response(buffer&& b)
    : m_buffer(std::move(b)) {}

public:

    template<typename T>
    T as() const {
        T t;
        buffer_input_archive iarch(m_buffer);
        iarch & t;
        return t;
    }

    template<typename T1, typename T2, typename ... Tn>
    auto as() const {
        std::tuple<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<Tn>...> t;
        buffer_input_archive iarch(m_buffer);
        iarch & t;
        return t;
    }

    template<typename T>
    operator T() const {
        return as<T>();
    }

};

}

#endif
