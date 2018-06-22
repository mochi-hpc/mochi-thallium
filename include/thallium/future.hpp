/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_FUTURE_HPP
#define __THALLIUM_FUTURE_HPP

#include <functional>
#include <vector>
#include <abt.h>

namespace thallium {

/**
 * @brief The future class wraps an ABT_future oject.
 * A future holds an array of pointers to objects of type T.
 * The future is not responsible for managing/freeing these objects.
 */
template<typename T>
class future {
	
    uint32_t                                    m_num_compartments;
	ABT_future                                  m_future;

    static void when_ready(void** arg) {
        auto f = static_cast<std::function<void(const std::vector<T*>&)>*>(arg[0]);
        auto n = static_cast<std::intptr_t>(arg[1]);
        std::vector<T*> v(n);
        for(unsigned i=0; i<n; i++) v[i] = static_cast<T*>(arg[i+2]);
        (*f)(v);
        delete f;
    }

	public:

    /**
     * @brief Type of the underlying native handle.
     */
    typedef ABT_future native_handle_type;

    /**
     * @brief Get the underlying native handle.
     *
     * @return The underlying native handle.
     */
    native_handle_type native_handle() const noexcept {
        return m_mutex;
    }

    /**
     * @brief Constructor.
     *
     * @param compartments Number of objects expected before the future is ready.
     */
    future(uint32_t compartments)
    : m_num_compartments(compartments) {
        ABT_future_create(compartments, nullptr, &m_future);
    }

    /**
     * @brief Constructor.
     *
     * @tparam F Type of callback.
     * @param compartments Number of objects expected before the future is ready.
     * @param cb_fun Function to call when the future becomes ready.
     */
    template<typename F>
    future(uint32_t compartments, F&& cb_fun)
    : m_num_compartments(compartments) {
        auto fp = new std::function<void(const std::vector<T*>&)>(std::forward<F>(cb_fun));
        std::intptr_t n = compartment+2;
        ABT_future_create(n, when_ready, &m_future);
        ABT_future_set(m_future,(void*)fp);
        ABT_future_set(m_future,(void*)n);
    }

    /**
     * @brief Copy constructor is deleted.
     */
    future(const future& other) = delete;

    /**
     * @brief Move constructor.
     */
    future(future&& other)
    : m_num_compartments(other.m_num_compartments),
      m_future(other.m_future) {
        other.m_future = ABT_FUTURE_NULL;
      }

    /**
     * @brief Copy assignment operator is deleted.
     */
    future& operator=(const future&) = delete;

    /**
     * @brief Move assignment operator is deleted.
     */
    future& operator=(future&&) = delete;

    /**
     * @brief Wait for the future to be ready.
     */
    void wait() {
        ABT_future_wait(m_future);
    }

    /**
     * @brief Test is the future is ready.
     */
    bool test() const {
        ABT_bool flag;
        ABT_future_test(m_future, &flag);
        return flag == ABT_TRUE ? true : false;
    }

    /**
     * @see future::test
     */
    operator bool() const {
        return test();
    }

    /**
     * @brief Set one of the future's values.
     *
     * @param value pointer to the value to set.
     */
    void set(T* value) {
        ABT_future_set(m_future, value);
    }

    /**
     * @brief Reset the future.
     */
    void reset() {
        ABT_future_reset(m_future);
        if(m_callback) {
            ABT_future_set(m_future,(void*)this);
        }
    }

    /**
     * @brief Destructor.
     */
    ~future() {
        ABT_future_free(&m_future);
    }
};

}

#endif /* end of include guard */
