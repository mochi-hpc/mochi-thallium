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

#else

#include <string>
#include <mercury_proc.h>
#include <type_traits>
#include <thallium/serialization/serialize.hpp>

namespace thallium {

class engine;
namespace detail {
    struct engine_impl;
}

using namespace std::string_literals;

/**
 * proc_output_archive wraps an hg_proc_t object and
 * offers the functionalities to serialize C++ objects into it.
 */
template<typename ... CtxArg>
class proc_output_archive : public output_archive {

private:

    hg_proc_t                          m_proc;
    std::tuple<CtxArg...>&             m_context;
    std::weak_ptr<detail::engine_impl> m_engine_impl;

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
     * \param p : reference to an hg_proc_t object.
     * \param e : thallium engine.
     */
    proc_output_archive(hg_proc_t p, std::tuple<CtxArg...>& context,
                        std::weak_ptr<detail::engine_impl> e)
    : m_proc(p), m_context(context), m_engine_impl(std::move(e)) {}

    proc_output_archive(hg_proc_t p, std::tuple<CtxArg...>& context)
    : m_proc(p), m_context(context), m_engine_impl() {}

    /**
     * Operator to add a C++ object of type T into the archive.
     * The object should either be a basic type, or an STL container
     * (in which case the appropriate header should
     * be included for this function to be properly instanciated), or
     * any object for which either a serialize member function or
     * a load member function has been provided.
     */
    template<typename T>
    inline proc_output_archive& operator&(T&& obj) {
        write_impl(std::forward<T>(obj), std::is_arithmetic<typename std::decay<T>::type>());
        return *this;
    }

    /**
     * @brief Parenthesis operator with one argument, equivalent to & operator.
     */
    template<typename T>
    inline proc_output_archive& operator()(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * @brief Parenthesis operator with multiple arguments.
     * ar(x,y,z) is equivalent to ar & x & y & z.
     */
    template<typename T, typename ... Targs>
    inline proc_output_archive& operator()(T&& obj, Targs&&... others) {
        (*this) & std::forward<T>(obj);
        return (*this)(std::forward<Targs>(others)...);
    }

    /**
     * Operator << is equivalent to operator &.
     * \see operator&
     */
    template<typename T>
    proc_output_archive& operator<<(T&& obj) {
        return (*this) & std::forward<T>(obj);
    }

    /**
     * Basic function to write count objects of type T into the buffer.
     * A memcopy is performed from the object's address to the buffer, so
     * the object should either be basic type or an object that can be
     * memcopied instead of calling a more elaborate serialize function.
     */
    template<typename T>
    inline void write(const T* const t, size_t count=1) {
        hg_return_t ret = hg_proc_memcpy(m_proc, const_cast<void*>(static_cast<const void*>(t)), count*sizeof(*t));
        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Error during serialization, hg_proc_memcpy returned"s + std::to_string(ret));
        }
    }

    /**
     * @brief Equivalent to write().
     */
    template<typename T>
    inline void copy(T* const t, size_t count=1) {
        write(t, count);
    }

    /**
     * @brief Returns the engine registered in the archive.
     *
     * @return The engine registered in the archive.
     */
    const std::weak_ptr<detail::engine_impl>& get_engine_impl() const {
        return m_engine_impl;
    }

    engine get_engine() const;

    /**
     * @brief Returns the hg_proc_t object handling the current serialization.
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

    template<typename ... CtxArg>
    inline engine proc_output_archive<CtxArg...>::get_engine() const {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        return engine(engine_impl);
    }

}

#endif
