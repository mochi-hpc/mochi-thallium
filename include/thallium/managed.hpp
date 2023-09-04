/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MANAGED_HPP
#define __THALLIUM_MANAGED_HPP

#include <iostream>

namespace thallium {

template <class T> class managed;

template <typename T, typename... Args>
managed<T> make_managed(Args&&... args);

/**
 * @brief The manager<T> class is used to automatically free
 * a resource holder (e.g. thread, task, xstream, pool, scheduler)
 * that would otherwise not free its underlying Argobots resource
 * upon destruction. This class acts like a smart pointer.
 *
 * @tparam T Type of resource holder.
 */
template <class T> class managed {
    friend T;

    template <class X, typename... Args>
    friend managed<X> make_managed(Args&&... args);

    template<typename ... Args>
    static managed<T> make(Args&&... args) {
        return managed<T>(T(std::forward<Args>(args)...));
    }

  private:
    T m_obj;

    managed(T&& obj)
    : m_obj(std::move(obj)) {}

  public:

    managed() = default;

    /**
     * @brief Deleted copy constructor.
     */
    managed(const managed&) = delete;

    /**
     * @brief Move constructor.
     */
    managed(managed&& other)
    : m_obj(std::move(other.m_obj)) {}

    /**
     * @brief Deleted copy-assignment operator.
     */
    managed& operator=(const managed&) = delete;

    /**
     * @brief Move-assignment operator.
     */
    managed& operator=(managed&& other) {
        m_obj.destroy();
        m_obj = std::move(other.m_obj);
        return *this;
    }

    /**
     * @brief Release the underlying resource.
     * This is equivalent to doing m = tl::managed<T>{};
     */
    void release() { m_obj.destroy(); }

    /**
     * @brief Destructor.
     */
    ~managed() { release(); }

    /**
     * @brief Dereference operator.
     */
    T& operator*() { return m_obj; }

    /**
     * @brief Pointer access operator.
     */
    T* operator->() { return &m_obj; }
};

/**
 * @brief Make a managed object by forwarding any passed parameters
 * to the constructor of the underlying resource type.
 *
 * @tparam Args Argument types
 * @param args Arguments
 */
template <typename T, typename... Args>
inline managed<T> make_managed(Args&&... args) {
    return managed<T>::make(std::forward<Args>(args)...);
}

} // namespace thallium

#endif
