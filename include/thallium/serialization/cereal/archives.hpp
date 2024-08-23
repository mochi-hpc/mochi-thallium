/*
 * Copyright (c) 2019 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef THALLIUM_CEREAL_ARCHIVES_BINARY_HPP
#define THALLIUM_CEREAL_ARCHIVES_BINARY_HPP

#include <string>
#include <cstring>
#include <mercury_proc.h>
#include <cereal/cereal.hpp>
#include <margo.h>
#include <thallium/exception.hpp>

namespace thallium {

    using namespace std::string_literals;

    class engine;

    template<typename ... CtxArg>
    class proc_output_archive :
        public cereal::OutputArchive<
            proc_output_archive<CtxArg...>,
            cereal::AllowEmptyClassElision>
    {

    public:

        proc_output_archive(hg_proc_t p, std::tuple<CtxArg...>& context,
                            margo_instance_id mid)
        : cereal::OutputArchive<proc_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_context(context)
        , m_mid(mid)
        {}

        proc_output_archive(hg_proc_t p, std::tuple<CtxArg...>& context)
        : cereal::OutputArchive<proc_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_context(context)
        {}

        ~proc_output_archive() = default;

        inline void write(const void* data, size_t size) {
            hg_return_t ret = hg_proc_memcpy(m_proc, const_cast<void*>(data), size);
            if(ret != HG_SUCCESS) {
                throw exception(
                    "Error during serialization, hg_proc_memcpy returned"s + std::to_string(ret));
            }
        }

        engine get_engine() const;

        hg_proc_t get_proc() const {
            return m_proc;
        }

        auto& get_context() {
            return m_context;
        }

        void* save_ptr(size_t size) {
            return hg_proc_save_ptr(m_proc, size);
        }

        void restore_ptr(void* buf, size_t size) {
            hg_proc_restore_ptr(m_proc, buf, size);
        }

    private:

        hg_proc_t              m_proc;
        std::tuple<CtxArg...>& m_context;
        margo_instance_id      m_mid = MARGO_INSTANCE_NULL;

    };

    template<typename ... CtxArg>
    class proc_input_archive :
        public cereal::InputArchive<
            proc_input_archive<CtxArg...>,
            cereal::AllowEmptyClassElision>
    {

    public:

        proc_input_archive(hg_proc_t p, std::tuple<CtxArg...>& context,
                           margo_instance_id mid)
        : cereal::InputArchive<proc_input_archive<CtxArg...>, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_context(context)
        , m_mid(mid)
        {}

        proc_input_archive(hg_proc_t p, std::tuple<CtxArg...>& context)
        : cereal::InputArchive<proc_input_archive<CtxArg...>, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_context(context)
        {}

        ~proc_input_archive() = default;

        inline void read(void* data, std::size_t size) {
            auto ret = hg_proc_memcpy(m_proc, static_cast<void*>(data), size);
            if(ret != HG_SUCCESS) {
                throw exception("Error during serialization, hg_proc_memcpy returned "s + std::to_string(ret));
            }
        }

        engine get_engine() const;

        hg_proc_t get_proc() const {
            return m_proc;
        }

        auto& get_context() {
            return m_context;
        }

        void* save_ptr(size_t size) {
            return hg_proc_save_ptr(m_proc, size);
        }

        void restore_ptr(void* buf, size_t size) {
            hg_proc_restore_ptr(m_proc, buf, size);
        }

    private:

        hg_proc_t              m_proc;
        std::tuple<CtxArg...>& m_context;
        margo_instance_id      m_mid;
    };

    template<class T, class... CtxArg> inline
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    CEREAL_SAVE_FUNCTION_NAME(proc_output_archive<CtxArg...> & ar, T const & t)
    {
        ar.write(std::addressof(t), sizeof(t));
    }

    template<class T, class... CtxArg> inline
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    CEREAL_LOAD_FUNCTION_NAME(proc_input_archive<CtxArg...>& ar, T & t)
    {
        t = T{};
        ar.read(std::addressof(t), sizeof(t));
    }

    template <class T, class... CtxArg> inline
    void CEREAL_SERIALIZE_FUNCTION_NAME(proc_output_archive<CtxArg...>& ar, cereal::NameValuePair<T>& t)
    {
        ar(t.value);
    }

    template <class T, class... CtxArg> inline
    void CEREAL_SERIALIZE_FUNCTION_NAME(proc_input_archive<CtxArg...>& ar, cereal::NameValuePair<T>& t)
    {
        ar(t.value);
    }

    template <class T, class... CtxArg> inline
    void CEREAL_SERIALIZE_FUNCTION_NAME(proc_output_archive<CtxArg...>& ar, cereal::SizeTag<T>& t)
    {
        ar(t.size);
    }

    template <class T, class... CtxArg> inline
    void CEREAL_SERIALIZE_FUNCTION_NAME(proc_input_archive<CtxArg...>& ar, cereal::SizeTag<T>& t)
    {
        ar(t.size);
    }

    template <class T, class... CtxArg> inline
    void CEREAL_SAVE_FUNCTION_NAME(proc_output_archive<CtxArg...>& ar, cereal::BinaryData<T> const & bd)
    {
        ar.write(bd.data, static_cast<std::size_t>(bd.size));
    }

    template <class T, class... CtxArg> inline
    void CEREAL_LOAD_FUNCTION_NAME(proc_input_archive<CtxArg...> & ar, cereal::BinaryData<T> & bd)
    {
        ar.read(bd.data, static_cast<std::size_t>(bd.size));
    }
}

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(thallium::proc_output_archive<>)
CEREAL_REGISTER_ARCHIVE(thallium::proc_input_archive<>)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(thallium::proc_input_archive<>, thallium::proc_output_archive<>)

#endif
