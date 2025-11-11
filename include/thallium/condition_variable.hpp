/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_CONDITION_VARIABLE_HPP
#define __THALLIUM_CONDITION_VARIABLE_HPP

#include <abt.h>
#include <mutex>
#include <chrono>
#include <ctime>
#include <thallium/exception.hpp>
#include <thallium/mutex.hpp>

namespace thallium {

/**
 * Exception class thrown by the condition_variable class.
 */
class condition_variable_exception : public exception {
  public:
    template <typename... Args>
    condition_variable_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_CV_EXCEPTION(__fun, __ret)                                          \
    condition_variable_exception(                                              \
        #__fun, " returned ", abt_error_get_name(__ret), " (",                 \
        abt_error_get_description(__ret), ") in ", __FILE__, ":", __LINE__);

#define TL_CV_ASSERT(__call)                                                   \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_CV_EXCEPTION(__call, __ret);                              \
        }                                                                      \
    }

/**
 * @brief Wrapper for Argobots' ABT_cond.
 */
class condition_variable {
    ABT_cond m_cond;

  public:
    /**
     * @brief Native handle type.
     */
    typedef ABT_cond native_handle_type;

    /**
     * @brief Returns the underlying ABT_cond handle.
     *
     * @return the underlying ABT_cond handle.
     */
    native_handle_type native_handle() const noexcept { return m_cond; }

    /**
     * @brief Constructor.
     */
    condition_variable() { TL_CV_ASSERT(ABT_cond_create(&m_cond)); }

    /**
     * @brief Destructor.
     */
    ~condition_variable() noexcept {
        if(m_cond != ABT_COND_NULL)
            ABT_cond_free(&m_cond);
    }

    /**
     * @brief Copy constructor is deleted.
     */
    condition_variable(const condition_variable&) = delete;

    /**
     * @brief Copy assignment operator is deleted.
     */
    condition_variable& operator=(const condition_variable&) = delete;

    /**
     * @brief Move assignment operator. If the left and right
     * operands are different, this will move the right
     * condition_variable's resources to the left condition_variable,
     * leaving the right one invalid.
     */
    condition_variable& operator=(condition_variable&& other) {
        if(this == &other)
            return *this;
        if(m_cond != ABT_COND_NULL) {
            TL_CV_ASSERT(ABT_cond_free(&m_cond));
        }
        m_cond       = other.m_cond;
        other.m_cond = ABT_COND_NULL;
        return *this;
    }

    /**
     * @brief Move constructor. This function will invalidate
     * the passed condition_variable.
     */
    condition_variable(condition_variable&& other) noexcept
    : m_cond(other.m_cond) {
        other.m_cond = ABT_COND_NULL;
    }

    /**
     * @brief Wait on a condition variable.
     *
     * @param lock Mutex to lock when the condition is satisfied.
     */
    void wait(std::unique_lock<mutex>& lock) {
        TL_CV_ASSERT(ABT_cond_wait(m_cond, lock.mutex()->native_handle()));
    }

    /**
     * @brief Wait on a condition variable until a predicate
     * becomes true.
     *
     * @tparam Predicate predicate type, must have parenthesis
     *         operator returning bool
     * @param lock Mutex to lock when the condition is satisfied.
     * @param pred Predicate to test.
     */
    template <class Predicate>
    void wait(std::unique_lock<mutex>& lock, Predicate&& pred) {
        while(!pred()) {
            wait(lock);
        }
    }

    /**
     * @brief Notify one waiter.
     */
    void notify_one() { TL_CV_ASSERT(ABT_cond_signal(m_cond)); }

    /**
     * @brief Notify all waiters.
     */
    void notify_all() { TL_CV_ASSERT(ABT_cond_broadcast(m_cond)); }

    /**
     * @brief Wait on a condition variable until a specific point in time.
     *
     * @param lock Mutex to lock when condition variable is satisfied.
     * @param abstime Date until which to wait.
     *
     * @return true if lock was acquired, false if timeout.
     */
    bool wait_until(std::unique_lock<mutex>& lock,
                    const struct timespec*   abstime) {
        int ret =
            ABT_cond_timedwait(m_cond, lock.mutex()->native_handle(), abstime);
        if(ABT_SUCCESS == ret) {
            return true;
        } else if(ABT_ERR_COND_TIMEDOUT == ret) {
            return false;
        } else {
            throw TL_CV_EXCEPTION(
                ABT_cond_timedwait(m_cond, lock.mutex()->native_handle(),
                                   abstime),
                ret);
        }
    }

    /**
     * @brief Wait on a condition variable until a specific point in time.
     *
     * @tparam Clock
     * @tparam Duration
     * @param lock Mutex to lock when the condition variable is satisfied.
     * @param abs_time Time point until which to wait.
     *
     * @return true if lock was acquired, false if timeout.
     */
    template <typename Clock, typename Duration>
    bool wait_until(std::unique_lock<mutex>& lock,
                    const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        using namespace std::chrono;

        // Convert time_point to timespec
        auto secs = time_point_cast<seconds>(abs_time);
        auto ns = duration_cast<nanoseconds>(abs_time - secs).count();

        struct timespec ts;
        ts.tv_sec  = secs.time_since_epoch().count();
        ts.tv_nsec = static_cast<long>(ns);

        return wait_until(lock, &ts);
    }

    /**
     * @brief Wait on a condition variable for a specified amount of time.
     *
     * @tparam Rep
     * @tparam Period
     * @tparam Clock
     * @param lock Mutex to lock when the condition variable is satisfied.
     * @param rel_time Duration to wait for.
     *
     * @return true if lock was acquired, false if timeout.
     */
    template <typename Rep, typename Period, typename Clock = std::chrono::steady_clock>
    bool wait_for(std::unique_lock<mutex>& lock,
                  const std::chrono::duration<Rep, Period>& rel_time)
        {
            using namespace std::chrono;

            // Compute absolute deadline as "now + rel_time"
            auto abs_time = Clock::now() + rel_time;
            return wait_until(lock, abs_time);
        }
};

} // namespace thallium

#undef TL_CV_EXCEPTION
#undef TL_CV_ASSERT

#endif /* end of include guard */
