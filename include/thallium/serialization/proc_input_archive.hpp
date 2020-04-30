/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP
#define __THALLIUM_BUFFER_INPUT_ARCHIVE_HPP

#include <thallium/config.hpp>

#ifdef THALLIUM_USE_CEREAL

#include <thallium/serialization/cereal/archives.hpp>

namespace thallium {

    using proc_input_archive = cereal_input_archive;

}


#else

#include <mercury_proc.h>
#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <string>
#include <thallium/serialization/serialize.hpp>

namespace thallium {

using namespace std::string_literals;

class engine;

/**
 * proc_input_archive wraps a hg_proc_t object and
 * offers the functionalities to deserialize its content
 * into C++ objects. It inherits from the input_archive
 * trait so that serialization methods know they have to
 * take data out of the buffer and into C++ objects.
 */
class proc_input_archive : public input_archive {

private:

    hg_proc_t     m_proc;
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
     * \param p : hg_proc_t from which to read.
     * \param engine : thallium engine.
     */
    proc_input_archive(hg_proc_t p, engine& e)
    : m_proc(p), m_engine(&e) {}

    proc_input_archive(hg_proc_t p)
    : m_proc(p), m_engine(nullptr) {}

    /**
     * Operator to get C++ objects of type T from the archive.
     * The object should either be a basic type, or an STL container
     * (in which case the appropriate thallium/serialization/stl/ header
     * should be included for this function to be properly instanciated),
     * or any object for which either a serialize member function or
     * a load member function has been provided.
     */
    template<typename T>
    inline proc_input_archive& operator&(T&& obj) {
        read_impl(std::forward<T>(obj), std::is_arithmetic<typename std::decay<T>::type>());
        return *this;
    }

    /**
     * @brief Parenthesis operator with one argument, equivalent to & operator.
     */
    template<typename T>
    inline proc_input_archive& operator()(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * @brief Parenthesis operator with multiple arguments.
     * ar(x,y,z) is equivalent to ar & x & y & z.
     */
    template<typename T, typename ... Targs>
    inline proc_input_archive& operator()(T&& obj, Targs&&... others) {
        (*this) & std::forward<T>(obj);
        return (*this)(std::forward<Targs>(others)...);
    }

    /**
     * Operator >> is equivalent to operator &.
     * \see operator&
     */
    template<typename T>
    proc_input_archive& operator>>(T&& obj) {
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
        hg_return_t ret = hg_proc_memcpy(m_proc, static_cast<void*>(t), count*sizeof(*t));
        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Error during serialization, hg_proc_memcpy returned"s + std::to_string(ret));
        }
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

    /**
     * @brief Returns the hg_proc_t object handling the current
     * serialization.
     */
    hg_proc_t get_proc() const {
        return m_proc;
    }
};

}
#endif
#endif
