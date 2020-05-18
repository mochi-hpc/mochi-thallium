/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_FUTURE_HPP
#define __THALLIUM_FUTURE_HPP

#include <abt.h>
#include <functional>
#include <thallium/exception.hpp>
#include <vector>

namespace thallium {

/**
 * Exception class thrown by the future class.
 */
class future_exception : public exception {
  public:
    template <typename... Args>
    future_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_FUTURE_EXCEPTION(__fun, __ret)                                      \
    future_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",    \
                     abt_error_get_description(__ret), ") in ", __FILE__, ":", \
                     __LINE__);

#define TL_FUTURE_ASSERT(__call)                                               \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_FUTURE_EXCEPTION(__call, __ret);                          \
        }                                                                      \
    }

/**
 * @brief The future class wraps an ABT_future oject.
 * A future holds an array of pointers to objects of type T.
 * The future is not responsible for managing/freeing these objects.
 */
template <typename T> class future {
    uint32_t   m_num_compartments;
    ABT_future m_future;

    static void when_ready(void** arg) {
        auto f =
            static_cast<std::function<void(const std::vector<T*>&)>*>(arg[0]);
        auto            n = reinterpret_cast<std::intptr_t>(arg[1]);
        std::vector<T*> v(n);
        for(unsigned i = 0; i < n; i++)
            v[i] = static_cast<T*>(arg[i + 2]);
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
    native_handle_type native_handle() const noexcept { return m_future; }

    /**
     * @brief Constructor.
     *
     * @param compartments Number of objects expected before the future is
     * ready.
     */
    future(uint32_t compartments)
    : m_num_compartments(compartments) {
        TL_FUTURE_ASSERT(
            ABT_future_create(compartments + 2, nullptr, &m_future));
        TL_FUTURE_ASSERT(ABT_future_set(m_future, nullptr));
        TL_FUTURE_ASSERT(ABT_future_set(m_future, (void*)this));
    }

    /**
     * @brief Constructor.
     *
     * @tparam F Type of callback.
     * @param compartments Number of objects expected before the future is
     * ready.
     * @param cb_fun Function to call when the future becomes ready.
     */
    template <typename F>
    future(uint32_t compartments, F&& cb_fun)
    : m_num_compartments(compartments) {
        auto fp = new std::function<void(const std::vector<T*>&)>(
            std::forward<F>(cb_fun));
        std::intptr_t n = compartments + 2;
        TL_FUTURE_ASSERT(ABT_future_create(n, when_ready, &m_future));
        TL_FUTURE_ASSERT(ABT_future_set(m_future, (void*)fp));
        TL_FUTURE_ASSERT(ABT_future_set(m_future, (void*)n));
    }

    /**
     * @brief Copy constructor is deleted.
     */
    future(const future& other) = delete;

    /**
     * @brief Move constructor.
     */
    future(future&& other)
    : m_num_compartments(other.m_num_compartments)
    , m_future(other.m_future) {
        other.m_future = ABT_FUTURE_NULL;
    }

    /**
     * @brief Copy assignment operator is deleted.
     */
    future& operator=(const future&) = delete;

    /**
     * @brief Move assignment operator.
     */
    future& operator=(future&& other) {
        if(this == &other) {
            return *this;
        }
        if(m_future != ABT_FUTURE_NULL) {
            TL_FUTURE_ASSERT(ABT_future_free(&m_future));
        }
        m_future       = other.m_future;
        other.m_future = ABT_FUTURE_NULL;
    }

    /**
     * @brief Wait for the future to be ready.
     */
    void wait() { TL_FUTURE_ASSERT(ABT_future_wait(m_future)); }

    /**
     * @brief Test is the future is ready.
     */
    bool test() const {
        ABT_bool flag;
        TL_FUTURE_ASSERT(ABT_future_test(m_future, &flag));
        return flag == ABT_TRUE ? true : false;
    }

    /**
     * @see future::test
     */
    operator bool() const { return test(); }

    /**
     * @brief Set one of the future's values.
     *
     * @param value pointer to the value to set.
     */
    void set(T* value) { TL_FUTURE_ASSERT(ABT_future_set(m_future, value)); }

    /**
     * @brief Destructor.
     */
    ~future() {
        if(m_future != ABT_FUTURE_NULL)
            ABT_future_free(&m_future);
    }
};

} // namespace thallium

#undef TL_FUTURE_EXCEPTION
#undef TL_FUTURE_ASSERT

#endif /* end of include guard */
