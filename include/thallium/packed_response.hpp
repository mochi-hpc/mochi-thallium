/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PACKED_RESPONSE_HPP
#define __THALLIUM_PACKED_RESPONSE_HPP

#include <thallium/buffer.hpp>
#ifdef USE_CEREAL
    #include <thallium/serialization/cereal/archives.hpp>
#else
    #include <thallium/serialization/serialize.hpp>
    #include <thallium/serialization/buffer_input_archive.hpp>
#endif

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
#ifdef USE_CEREAL
        cereal_input_archive iarch(m_buffer, *m_engine);
        iarch(t);
#else
        buffer_input_archive iarch(m_buffer, *m_engine);
        iarch & t;
#endif
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
        std::tuple<typename std::decay<T1>::type, 
            typename std::decay<T2>::type, 
            typename std::decay_t<Tn>::type...> t;
#ifdef USE_CEREAL
        std::function<void(T1, T2, Tn...)> deserialize = [this](T1&& t1, T2&& t2, Tn&&... tn) {
            cereal_input_archive arch(m_buffer, *m_engine);
            arch(std::forward<T1>(t1),
                 std::forward<T2>(t2),
                 std::forward<Tn>(tn)...);
        };
        apply_function_to_tuple(deserialize, t);
#else
        buffer_input_archive iarch(m_buffer, *m_engine);
        iarch & t;
#endif
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
