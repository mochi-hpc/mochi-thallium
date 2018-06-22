/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MANAGED_HPP
#define __THALLIUM_MANAGED_HPP

#include <iostream>

namespace thallium {

template<class T>
class managed {

    friend T;

    private:

    T m_obj;

    template<typename ... Args>
    managed(Args&&... args)
    : m_obj(std::forward<Args>(args)...) {}

    public:

    managed(const managed&)             = delete;

    managed(managed&& other)
    : m_obj(std::move(other.m_obj)) {}

    managed& operator=(const managed&)  = delete;

    managed& operator=(managed&& other) {
        m_obj = std::move(other.m_obj);
    }

    ~managed() {
        m_obj.destroy();
    }

    T& operator*() {
        return m_obj;
    }

    T* operator->() {
        return &m_obj;
    }
};

}

#endif
