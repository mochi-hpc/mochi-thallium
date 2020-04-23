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

namespace thallium {

    using namespace std::string_literals;

    class engine;

    class cereal_output_archive : public cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>
    {
    
    public:

        cereal_output_archive(hg_proc_t p, engine& e)
        : cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_engine(&e)
        {}

        cereal_output_archive(hg_proc_t p)
        : cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_engine(nullptr)
        {}

        ~cereal_output_archive() = default;

        inline void write(const void* data, size_t size) {
            hg_return_t ret = hg_proc_memcpy(m_proc, const_cast<void*>(data), size);
            if(ret != HG_SUCCESS) {
                throw std::runtime_error("Error during serialization, hg_proc_memcpy returned"s + std::to_string(ret));
            }
        }

        engine& get_engine() const {
            return *m_engine;
        }

    private:

        hg_proc_t m_proc;
        engine*   m_engine;

    };

    class cereal_input_archive : public cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>
    {
    
    public:

        cereal_input_archive(hg_proc_t p, engine& e)
        : cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_engine(&e)
        {}

        cereal_input_archive(hg_proc_t p)
        : cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>(this)
        , m_proc(p)
        , m_engine(nullptr)
        {}

        ~cereal_input_archive() = default;

        inline void read(void* data, std::size_t size) {
            hg_return_t ret = hg_proc_memcpy(m_proc, static_cast<void*>(data), size);
            if(ret != HG_SUCCESS) {
                throw std::runtime_error("Error during serialization, hg_proc_memcpy returned"s + std::to_string(ret));
            }
        }

        engine& get_engine() const {
            return *m_engine;
        }

    private:

        hg_proc_t m_proc;
        engine*   m_engine;
    };

    template<class T> inline
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    CEREAL_SAVE_FUNCTION_NAME(cereal_output_archive & ar, T const & t)
    {
        ar.write(std::addressof(t), sizeof(t));
    }

    template<class T> inline
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    CEREAL_LOAD_FUNCTION_NAME(cereal_input_archive & ar, T & t)
    {
        ar.read(std::addressof(t), sizeof(t));
    }

    template <class Archive, class T> inline
    CEREAL_ARCHIVE_RESTRICT(cereal_input_archive, cereal_output_archive)
    CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, cereal::NameValuePair<T>& t)
    {
        ar(t.value);
    }

    template <class Archive, class T> inline
    CEREAL_ARCHIVE_RESTRICT(cereal_input_archive, cereal_output_archive)
    CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, cereal::SizeTag<T>& t)
    {
        ar(t.size);
    }

    template <class T> inline
    void CEREAL_SAVE_FUNCTION_NAME(cereal_output_archive& ar, cereal::BinaryData<T> const & bd)
    {
        ar.write(bd.data, static_cast<std::size_t>(bd.size));
    }

    template <class T> inline
    void CEREAL_LOAD_FUNCTION_NAME(cereal_input_archive & ar, cereal::BinaryData<T> & bd)
    {
        ar.read(bd.data, static_cast<std::size_t>(bd.size));
    }
}

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(thallium::cereal_output_archive)
CEREAL_REGISTER_ARCHIVE(thallium::cereal_input_archive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(thallium::cereal_input_archive, thallium::cereal_output_archive)

#endif
