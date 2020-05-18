/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_BARRIER_HPP
#define __THALLIUM_BARRIER_HPP

#include <abt.h>
#include <thallium/abt_errors.hpp>
#include <thallium/exception.hpp>

namespace thallium {

/**
 * Exception class thrown by the barrier class.
 */
class barrier_exception : public exception {
  public:
    template <typename... Args>
    barrier_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_BARRIER_EXCEPTION(__fun, __ret)                                     \
    barrier_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",   \
                      abt_error_get_description(__ret), ") in ", __FILE__,     \
                      ":", __LINE__);

#define TL_BARRIER_ASSERT(__call)                                              \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_BARRIER_EXCEPTION(__call, __ret);                         \
        }                                                                      \
    }

/**
 * @brief Wrapper for Argobots' ABT_barrier.
 */
class barrier {
    ABT_barrier m_barrier;

  public:
    /**
     * @brief Native handle type (ABT_barrier)
     */
    typedef ABT_barrier native_handle_type;

    /**
     * @brief Constructor.
     *
     * @param num_waiters Number of waiters.
     */
    explicit barrier(uint32_t num_waiters) {
        TL_BARRIER_ASSERT(ABT_barrier_create(num_waiters, &m_barrier));
    }

    /**
     * @brief Copy constructor is deleted.
     */
    barrier(const barrier& other) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    barrier& operator=(const barrier& other) = delete;

    /**
     * @brief Move assignment operator.
     *
     * If the right and left operands are different,
     * this method will free the left operand's resource (if
     * necessary), and assign it the right operand's resource.
     * The right operand will be invalidated.
     */
    barrier& operator=(barrier&& other) {
        if(this == &other)
            return *this;
        if(m_barrier != ABT_BARRIER_NULL) {
            TL_BARRIER_ASSERT(ABT_barrier_free(&m_barrier));
        }
        m_barrier       = other.m_barrier;
        other.m_barrier = ABT_BARRIER_NULL;
        return *this;
    }

    /**
     * @brief Move constructor. The right operand
     * will be invalidated.
     *
     * @param other barrier object to move from.
     */
    barrier(barrier&& other)
    : m_barrier(other.m_barrier) {
        other.m_barrier = ABT_BARRIER_NULL;
    }

    /**
     * @brief Destructor.
     */
    ~barrier() {
        if(m_barrier != ABT_BARRIER_NULL)
            ABT_barrier_free(&m_barrier);
    }

    /**
     * @brief Reinitializes the barrier for a given
     * number of waiters.
     *
     * @param num_waiters Number of waiters.
     */
    void reinit(uint32_t num_waiters) {
        if(m_barrier == ABT_BARRIER_NULL) {
            TL_BARRIER_ASSERT(ABT_barrier_create(num_waiters, &m_barrier));
        } else {
            TL_BARRIER_ASSERT(ABT_barrier_reinit(m_barrier, num_waiters));
        }
    }

    /**
     * @brief Waits on the barrier.
     */
    void wait() { TL_BARRIER_ASSERT(ABT_barrier_wait(m_barrier)); }

    /**
     * @brief Get the number of waiters that the barrier
     * is expecting (passed to the constructor or to reinit).
     *
     * @return The number of waiters.
     */
    uint32_t get_num_waiters() const {
        uint32_t n;
        TL_BARRIER_ASSERT(ABT_barrier_get_num_waiters(m_barrier, &n));
        return n;
    }

    /**
     * @brief Get the underlying ABT_barrier handle.
     *
     * @return the underlying ABT_barrier handle.
     */
    native_handle_type native_handle() const noexcept { return m_barrier; }
};

} // namespace thallium

#undef TL_BARRIER_EXCEPTION
#undef TL_BARRIER_ASSERT

#endif /* end of include guard */
