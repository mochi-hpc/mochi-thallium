/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_ES_BARRIER_HPP
#define __THALLIUM_ES_BARRIER_HPP

#include <abt.h>
#include <thallium/abt_errors.hpp>
#include <thallium/exception.hpp>

namespace thallium {

/**
 * Exception class thrown by the xstream_barrier class.
 */
class xstream_barrier_exception : public exception {
  public:
    template <typename... Args>
    xstream_barrier_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_ES_BARRIER_EXCEPTION(__fun, __ret)                                  \
    xstream_barrier_exception(#__fun, " returned ", abt_error_get_name(__ret), \
                              " (", abt_error_get_description(__ret), ") in ", \
                              __FILE__, ":", __LINE__);

#define TL_ES_BARRIER_ASSERT(__call)                                           \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_ES_BARRIER_EXCEPTION(__call, __ret);                      \
        }                                                                      \
    }

/**
 * @brief Wrapper for Argobots' ABT_xstream_barrier.
 */
class xstream_barrier {
    ABT_xstream_barrier m_es_barrier;

  public:
    /**
     * @brief Native handle type (ABT_xstream_barrier)
     */
    typedef ABT_xstream_barrier native_handle_type;

    /**
     * @brief Constructor.
     *
     * @param num_waiters Number of waiters.
     */
    explicit xstream_barrier(uint32_t num_waiters) {
        TL_ES_BARRIER_ASSERT(
            ABT_xstream_barrier_create(num_waiters, &m_es_barrier));
    }

    /**
     * @brief Copy constructor is deleted.
     */
    xstream_barrier(const xstream_barrier& other) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    xstream_barrier& operator=(const xstream_barrier& other) = delete;

    /**
     * @brief Move assignment operator.
     *
     * If the right and left operands are different,
     * this method will free the left operand's resource (if
     * necessary), and assign it the right operand's resource.
     * The right operand will be invalidated.
     */
    xstream_barrier& operator=(xstream_barrier&& other) {
        if(this == &other)
            return *this;
        if(m_es_barrier != ABT_XSTREAM_BARRIER_NULL) {
            TL_ES_BARRIER_ASSERT(ABT_xstream_barrier_free(&m_es_barrier));
        }
        m_es_barrier       = other.m_es_barrier;
        other.m_es_barrier = ABT_XSTREAM_BARRIER_NULL;
        return *this;
    }

    /**
     * @brief Move constructor. The right operand
     * will be invalidated.
     *
     * @param other barrier object to move from.
     */
    xstream_barrier(xstream_barrier&& other)
    : m_es_barrier(other.m_es_barrier) {
        other.m_es_barrier = ABT_XSTREAM_BARRIER_NULL;
    }

    /**
     * @brief Destructor.
     */
    ~xstream_barrier() {
        if(m_es_barrier != ABT_XSTREAM_BARRIER_NULL)
            ABT_xstream_barrier_free(&m_es_barrier);
    }

    /**
     * @brief Waits on the barrier.
     */
    void wait() {
        TL_ES_BARRIER_ASSERT(ABT_xstream_barrier_wait(m_es_barrier));
    }

    /**
     * @brief Get the underlying ABT_xstream_barrier handle.
     *
     * @return the underlying ABT_xstream_barrier handle.
     */
    native_handle_type native_handle() const noexcept { return m_es_barrier; }
};

} // namespace thallium

#undef TL_ES_BARRIER_EXCEPTION
#undef TL_ES_BARRIER_ASSERT

#endif /* end of include guard */
