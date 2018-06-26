/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MANAGED_HPP
#define __THALLIUM_MANAGED_HPP

#include <iostream>

namespace thallium {

/**
 * @brief The manager<T> class is used to automatically free
 * a resource holder (e.g. thread, task, xstream, pool, scheduler)
 * that would otherwise not free its underlying Argobots resource
 * upon destruction. This class acts like a smart pointer.
 *
 * @tparam T Type of resource holder.
 */
template<class T>
class managed {

    friend T;

    private:

    T m_obj;

    /**
     * @brief Constructor, forwards any passed parameters
     * to the constructor of the underlying resource type.
     *
     * @tparam Args Argument types
     * @param args Arguments
     */
    template<typename ... Args>
    managed(Args&&... args)
    : m_obj(std::forward<Args>(args)...) {}

    public:

    /**
     * @brief Deleted copy constructor.
     */
    managed(const managed&)             = delete;

    /**
     * @brief Move constructor.
     */
    managed(managed&& other)
    : m_obj(std::move(other.m_obj)) {}

    /**
     * @brief Deleted copy-assignment operator.
     */
    managed& operator=(const managed&)  = delete;

    /**
     * @brief Move-assignment operator.
     */
    managed& operator=(managed&& other) {
        m_obj = std::move(other.m_obj);
    }

    /**
     * @brief Destructor.
     */
    ~managed() {
        m_obj.destroy();
    }

    /**
     * @brief Dereference operator.
     */
    T& operator*() {
        return m_obj;
    }

    /**
     * @brief Pointer access operator.
     */
    T* operator->() {
        return &m_obj;
    }
};

}

#endif
