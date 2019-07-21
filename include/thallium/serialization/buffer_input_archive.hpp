/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP

#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <thallium/serialization/serialize.hpp>
#include <thallium/buffer.hpp>

namespace thallium {

class engine;

/**
 * buffer_input_archive wraps a buffer object and
 * offers the functionalities to deserialize its content
 * into C++ objects. It inherits from the input_archive
 * trait so that serialization methods know they have to
 * take data out of the buffer and into C++ objects.
 */
class buffer_input_archive : public input_archive {

private:

    const buffer& m_buffer;
    std::size_t   m_pos;
    engine*       m_engine;

    template<typename T, bool b>
    inline void read_impl(T&& t, const std::integral_constant<bool, b>&) {
        load(*this,std::forward<T>(t));
    }

    template<typename T>
    inline void read_impl(T&& t, const std::true_type&) {
        read(&t);
    }

public:

    /**
     * Constructor.
     *
     * \param b : reference to a buffer from which to read.
     * \warning The buffer is held by reference so the life span of
     * the buffer_input_archive instance should be shorter than that
     * of the buffer.
     */
    buffer_input_archive(const buffer& b, engine& e)
    : m_buffer(b), m_pos(0), m_engine(&e) {}

    buffer_input_archive(const buffer& b)
    : m_buffer(b), m_pos(0), m_engine(nullptr) {}

    /**
     * Operator to get C++ objects of type T from the archive.
     * The object should either be a basic type, or an STL container
     * (in which case the appropriate thallium/serialization/stl/ header
     * should be included for this function to be properly instanciated),
     * or any object for which either a serialize member function or
     * a load member function has been provided.
     */
    template<typename T>
    inline buffer_input_archive& operator&(T&& obj) {
        read_impl(std::forward<T>(obj), std::is_arithmetic<typename std::decay<T>::type>());
        return *this;
    }

    /**
     * @brief Parenthesis operator with one argument, equivalent to & operator.
     */
    template<typename T>
    inline buffer_input_archive& operator()(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * @brief Parenthesis operator with multiple arguments.
     * ar(x,y,z) is equivalent to ar & x & y & z.
     */
    template<typename T, typename ... Targs>
    inline buffer_input_archive& operator()(T&& obj, Targs&&... others) {
        (*this) & std::forward<T>(obj);
        return (*this)(std::forward<Targs>(others)...);
    }

    /**
     * Operator >> is equivalent to operator &.
     * \see operator&
     */
    template<typename T>
    buffer_input_archive& operator>>(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * Basic function to read count objects of type T from the buffer.
     * A memcopy is performed from the buffer to the object, so the
     * object should either be a basic type or an object that can be
     * memcopied instead of calling a more elaborate serialize function.
     */
    template<typename T>
    inline void read(T* t, std::size_t count=1) {
        if(m_pos + count*sizeof(T) > m_buffer.size()) {
            throw std::runtime_error("Reading beyond buffer size");
        }
        std::memcpy((void*)t,(const void*)(m_buffer.data() + m_pos),count*sizeof(T));
        m_pos += count*sizeof(T);
    }

    /**
     * @brief Equivalent to read().
     */
    template<typename T>
    inline void copy(T* t, std::size_t count=1) {
        read(t, count);
    }

    /**
     * @brief Returns the engine registered in the archive.
     *
     * @return The engine registered in the archive.
     */
    engine& get_engine() const {
        return *m_engine;
    }
};

}
#endif
