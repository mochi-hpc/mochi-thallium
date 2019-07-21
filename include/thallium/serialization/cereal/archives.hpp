/*
 * Copyright (c) 2019 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef THALLIUM_CEREAL_ARCHIVES_BINARY_HPP
#define THALLIUM_CEREAL_ARCHIVES_BINARY_HPP

#include <cstring>
#include <thallium/buffer.hpp>
#include <cereal/cereal.hpp>

namespace thallium {

    class engine;

    class cereal_output_archive : public cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>
    {
    
    public:

        cereal_output_archive(buffer& b, engine& e)
        : cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_buffer(b)
        , m_pos(0)
        , m_engine(&e)
        { 
            m_buffer.resize(0);
        }

        cereal_output_archive(buffer& b)
        : cereal::OutputArchive<cereal_output_archive, cereal::AllowEmptyClassElision>(this)
        , m_buffer(b)
        , m_pos(0)
        , m_engine(nullptr)
        { 
            m_buffer.resize(0);
        }

        ~cereal_output_archive() = default;

        inline void write(void* const data, size_t size) {
            if(m_pos+size > m_buffer.size()) {
                if(m_pos+size > m_buffer.capacity()) {
                    m_buffer.reserve(m_buffer.capacity()*2);
                }
                m_buffer.resize(m_pos+size);
            }
            memcpy((void*)(m_buffer.data() + m_pos),(void*)data, size);
            m_pos += size;
        }

        engine& get_engine() const {
            return *m_engine;
        }

    private:

        buffer&     m_buffer;
        std::size_t m_pos;
        engine*     m_engine;

    };

    class cereal_input_archive : public cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>
    {
    
    public:

        cereal_input_archive(const buffer& b, engine& e)
        : cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>(this)
        , m_buffer(b)
        , m_pos(0)
        , m_engine(&e)
        {}

        cereal_input_archive(buffer& b)
        : cereal::InputArchive<cereal_input_archive, cereal::AllowEmptyClassElision>(this)
        , m_buffer(b)
        , m_pos(0)
        , m_engine(nullptr)
        {}

        ~cereal_input_archive() = default;

        inline void read(void* data, std::size_t size) {
            if(m_pos + size > m_buffer.size()) {
                throw std::runtime_error("Reading beyond buffer size");
            }
            std::memcpy((void*)data,(const void*)(m_buffer.data() + m_pos), size);
            m_pos += size;
        }

        engine& get_engine() const {
            return *m_engine;
        }

    private:

        const buffer& m_buffer;
        std::size_t   m_pos;
        engine*       m_engine;
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
