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
class async_response;

/**
 * @brief packed_response objects are created as a reponse to
 * an RPC. They can be used to extract the response from the
 * RPC if the RPC sent one.
 */
class packed_response {

    friend class callable_remote_procedure;
    friend class async_response;

private:

    engine* m_engine;
    buffer  m_buffer;

    /**
     * @brief Constructor. Made private since packed_response
     * objects are created by callable_remote_procedure only.
     *
     * @param b Buffer containing the result of an RPC.
     * @param e Engine associated with the RPC.
     */
    packed_response(buffer&& b, engine& e)
    : m_engine(&e), m_buffer(std::move(b)) {}

public:

    /**
     * @brief Converts the buffer into the requested object.
     *
     * @tparam T Type into which to convert the content of the buffer.
     *
     * @return Buffer converted into the desired type.
     */
    template<typename T>
    T as() const {
        T t;
        buffer_input_archive iarch(m_buffer, *m_engine);
        iarch & t;
        return t;
    }

    /**
     * @brief Converts the content of the buffer into a std::tuple
     * of types T1, T2, ... Tn.
     *
     * This function allows to do something like the following:
     * int x;
     * double y;
     * std::tie(x,y) = pack.as<int,double>();
     *
     * @tparam T1 First type of the tuple.
     * @tparam T2 Second type of the tuple.
     * @tparam Tn Other types of the tuple.
     *
     * @return buffer content converted into the desired std::tuple.
     */
    template<typename T1, typename T2, typename ... Tn>
    auto as() const {
        std::tuple<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<Tn>...> t;
        buffer_input_archive iarch(m_buffer);
        iarch & t;
        return t;
    }

    /**
     * @brief Converts the content of the packed_response into
     * the desired object type. Allows to cast the packed_response
     * into the desired object type.
     *
     * @tparam T Type into which to convert the response.
     *
     * @return An object of the desired type.
     */
    template<typename T>
    operator T() const {
        return as<T>();
    }

};

}

#endif
