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

#else

#include <mercury_proc.h>
#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <string>
#include <memory>
#include <thallium/serialization/serialize.hpp>

namespace thallium {

using namespace std::string_literals;

class engine;

namespace detail {
    struct engine_impl;
}

/**
 * proc_input_archive wraps a hg_proc_t object and
 * offers the functionalities to deserialize its content
 * into C++ objects. It inherits from the input_archive
 * trait so that serialization methods know they have to
 * take data out of the buffer and into C++ objects.
 */
template<typename ... CtxArg>
class proc_input_archive : public input_archive {

private:

    hg_proc_t                           m_proc;
    std::tuple<CtxArg...>&              m_context;
    std::weak_ptr<detail::engine_impl>  m_engine_impl;

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
    proc_input_archive(hg_proc_t p, std::tuple<CtxArg...>& context,
                       const std::weak_ptr<detail::engine_impl>& e)
    : m_proc(p), m_context(context), m_engine_impl(e) {}

    proc_input_archive(hg_proc_t p, std::tuple<CtxArg...>& context)
    : m_proc(p), m_context(context), m_engine_impl() {}

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
     * @brief Returns the engine impl registered in the archive.
     *
     * @return The engine impl registered in the archive.
     */
    const std::weak_ptr<detail::engine_impl>& get_engine_impl() const {
        return m_engine_impl;
    }

    engine get_engine() const;

    /**
     * @brief Returns the hg_proc_t object handling the current
     * serialization.
     */
    hg_proc_t get_proc() const {
        return m_proc;
    }

    /**
     * Retrieve context objects bound with the archive.
     */
    auto& get_context() {
        return m_context;
    }

    /**
     * @brief Calls hg_proc_save_ptr for manual encoding into the Mercury buffer.
     */
    void* save_ptr(size_t size) {
        return hg_proc_save_ptr(m_proc, size);
    }

    /**
     * @brief Restore pointer after manual encoding.
     */
    void restore_ptr(void* buf, size_t size) {
        hg_proc_restore_ptr(m_proc, buf, size);
    }
};

}
#endif

#include <thallium/engine.hpp>

namespace thallium {

    template<typename... CtxArg>
    inline engine proc_input_archive<CtxArg...>::get_engine() const {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        return engine(engine_impl);
    }

}

#endif
