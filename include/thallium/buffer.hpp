/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BUFFER_HPP
#define __THALLIUM_BUFFER_HPP

#include <stdlib.h>
#include <string.h>

namespace thallium {

class buffer {

    char*  m_data     = nullptr;
    size_t m_size     = 0;
    size_t m_capacity = 0;

public:

    buffer() = default;

    buffer(size_t initialSize) {
        resize(initialSize);
    }

    ~buffer() {
        if(m_data != nullptr)
            free(m_data);
    }

    buffer(const buffer& other) {
        if(other.m_data != nullptr) {
            m_data = static_cast<char*>(malloc(other.m_size));
            m_size = other.m_size;
            m_capacity = other.m_size;
            memcpy(m_data, other.m_data, m_size);
        } else {
            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
        }
    }

    buffer(buffer&& other) {
        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    buffer& operator=(const buffer& other) {
        if(&other == this) return *this;
        if(m_data != nullptr)
            free(m_data);
        if(other.m_data != nullptr) {
            m_data = static_cast<char*>(malloc(other.m_size));
            m_size = other.m_size;
            m_capacity = other.m_size;
            memcpy(m_data, other.m_data, m_size);
        } else {
            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
        }
        return *this;
    }

    buffer& operator=(buffer&& other) {
        if(&other == this) return *this;
        if(m_data != nullptr)
            free(m_data);
        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        return *this;
    }

    const char* data() const {
        return m_data;
    }

    char* data() {
        return m_data;
    }

    size_t size() const {
        return m_size;
    }

    size_t capacity() const {
        return m_capacity;
    }

    void resize(size_t newSize) {
        if(m_capacity == 0) {
            m_data = static_cast<char*>(malloc(newSize));
            m_size = newSize;
            m_capacity = newSize;
        } else if(m_capacity >= newSize) {
            m_size = newSize;
        } else { // capacity not 0 but too small
            while(m_capacity < newSize) m_capacity *= 2;
            m_data = static_cast<char*>(realloc(m_data, m_capacity));
            m_size = newSize;
        }
    }

    void reserve(size_t newCapacity) {
        if(newCapacity <= m_capacity)
            return;
        m_capacity = newCapacity;
        m_data = static_cast<char*>(realloc(m_data, m_capacity));
    }
};
    
}

#endif
