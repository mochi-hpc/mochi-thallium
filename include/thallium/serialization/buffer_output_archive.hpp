/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BUFFER_OUTPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_OUTPUT_ARCHIVE_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL

#include <thallium/serialization/cereal/archives.hpp>

namespace thallium {

        using buffer_output_archive = cereal_output_archive;

}

#else

#include <type_traits>
#include <thallium/serialization/serialize.hpp>
#include <thallium/buffer.hpp>

namespace thallium {

class engine;

/**
 * buffer_output_archive wraps and hg::buffer object and
 * offers the functionalities to serialize C++ objects into
 * the buffer. The buffer is resized down to 0 when creating
 * the archive and will be extended back to an appropriate size
 * as C++ objects are serialized into it.
 */
class buffer_output_archive : public output_archive {

private:

    buffer&     m_buffer;
    std::size_t m_pos;
    engine*     m_engine;

    template<typename T, bool b>
    inline void write_impl(T&& t, const std::integral_constant<bool, b>&) {
        save(*this,std::forward<T>(t));
    }

    template<typename T>
    inline void write_impl(T&& t, const std::true_type&) {
        write((char*)&t,sizeof(T));
    }

public:

    /**
     * Constructor.
     * 
     * \param b : reference to a buffer into which to write.
     * \warning The buffer is held by reference so the life span
     * of the buffer_output_archive instance should be shorter than
     * that of the buffer itself.
     */
    buffer_output_archive(buffer& b, engine& e)
    : m_buffer(b), m_pos(0), m_engine(&e) {
        m_buffer.resize(0);
    }

    buffer_output_archive(buffer& b)
    : m_buffer(b), m_pos(0), m_engine(nullptr) {
        m_buffer.resize(0);
    }

    /**
     * Operator to add a C++ object of type T into the archive.
     * The object should either be a basic type, or an STL container
     * (in which case the appropriate hgcxx/hg_stl/stl_* header should
     * be included for this function to be properly instanciated), or
     * any object for which either a serialize member function or
     * a load member function has been provided.
     */
    template<typename T>
    inline buffer_output_archive& operator&(T&& obj) {
        write_impl(std::forward<T>(obj), std::is_arithmetic<typename std::decay<T>::type>());
        return *this;
    }

    /**
     * @brief Parenthesis operator with one argument, equivalent to & operator.
     */
    template<typename T>
    inline buffer_output_archive& operator()(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * @brief Parenthesis operator with multiple arguments.
     * ar(x,y,z) is equivalent to ar & x & y & z.
     */
    template<typename T, typename ... Targs>
    inline buffer_output_archive& operator()(T&& obj, Targs&&... others) {
        (*this) & std::forward<T>(obj);
        return (*this)(std::forward<Targs>(others)...);
    }

    /**
     * Operator << is equivalent to operator &.
     * \see operator&
     */
    template<typename T>
    buffer_output_archive& operator<<(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * Basic function to write count objects of type T into the buffer.
     * A memcopy is performed from the object's address to the buffer, so
     * the object should either be basic type or an object that can be
     * memcopied instead of calling a more elaborate serialize function.
     */
    template<typename T>
    inline void write(T* const t, size_t count=1) {
        size_t s = count*sizeof(T);
        if(m_pos+s > m_buffer.size()) {
            if(m_pos+s > m_buffer.capacity()) {
                m_buffer.reserve(m_buffer.capacity()*2);
            }
            m_buffer.resize(m_pos+s);
        }
        memcpy((void*)(m_buffer.data() + m_pos),(void*)t,s);
        m_pos += s;
    }

    /**
     * @brief Equivalent to write().
     */
    template<typename T>
    inline void copy(T* const t, size_t count=1) {
        write(t, count);
    }
};

}

#endif
#endif
