/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_TIMER_HPP
#define __THALLIUM_TIMER_HPP

#include <abt.h>
#include <thallium/abt_errors.hpp>
#include <thallium/exception.hpp>

namespace thallium {

/**
 * Exception class thrown by the timer class.
 */
class timer_exception : public exception {
  public:
    template <typename... Args>
    timer_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_TIMER_EXCEPTION(__fun, __ret)                                       \
    timer_exception(#__fun, " returned ", abt_error_get_name(__ret), " (",     \
                    abt_error_get_description(__ret), ") in ", __FILE__, ":",  \
                    __LINE__);

#define TL_TIMER_ASSERT(__call)                                                \
    {                                                                          \
        int __ret = __call;                                                    \
        if(__ret != ABT_SUCCESS) {                                             \
            throw TL_TIMER_EXCEPTION(__call, __ret);                           \
        }                                                                      \
    }

/**
 * @brief Wrapper for Argobots' ABT_timer.
 */
class timer {
    ABT_timer m_timer;

  public:
    /**
     * @brief Native handle type.
     */
    typedef ABT_timer native_handle_type;

    /**
     * @brief Get elapsed wall clock time.
     *
     * @return elapsed wall clock time.
     */
    static double wtime() { return ABT_get_wtime(); }

    /**
     * @brief Return the overhead of the timer class.
     *
     * @return the overhead time when measuring the elapsed
     * time with the timer class. It computes the time difference
     * in consecutive calls of timer::start() and timer::stop().
     * The resolution of overhead time is at least a unit of microsecond.
     */
    static double overhead() {
        timer     t;
        int       i;
        const int iter = 5000;
        double    secs, sum = 0.0;
        for(i = 0; i < iter; i++) {
            t.start();
            t.stop();
            secs = t.read();
            sum += secs;
        }
        return sum / iter;
    }

    /**
     * @brief Get the native handle.
     *
     * @return the native handle.
     */
    native_handle_type native_handle() const { return m_timer; }

    /**
     * @brief Constructor.
     */
    timer() { TL_TIMER_ASSERT(ABT_timer_create(&m_timer)); }

    /**
     * @brief Destructor.
     */
    ~timer() { ABT_timer_free(&m_timer); }

    /**
     * @brief Copy constructor.
     */
    timer(const timer& other)
    : m_timer(ABT_TIMER_NULL) {
        if(other.m_timer != ABT_TIMER_NULL) {
            TL_TIMER_ASSERT(ABT_timer_dup(other.m_timer, &m_timer));
        }
    }

    /**
     * @brief Move constructor.
     */
    timer(timer&& other)
    : m_timer(other.m_timer) {
        other.m_timer = ABT_TIMER_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    timer& operator=(const timer& other) {
        if(this == &other)
            return *this;
        if(m_timer != ABT_TIMER_NULL) {
            TL_TIMER_ASSERT(ABT_timer_free(&m_timer));
            m_timer = ABT_TIMER_NULL;
        }
        if(other.m_timer != ABT_TIMER_NULL) {
            TL_TIMER_ASSERT(ABT_timer_dup(other.m_timer, &m_timer));
        }
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    timer& operator=(timer&& other) {
        if(this == &other)
            return *this;
        if(m_timer != ABT_TIMER_NULL) {
            TL_TIMER_ASSERT(ABT_timer_free(&m_timer));
            m_timer = ABT_TIMER_NULL;
        }
        m_timer       = other.m_timer;
        other.m_timer = ABT_TIMER_NULL;
        return *this;
    }

    /**
     * @brief Starts the timer.
     */
    void start() { TL_TIMER_ASSERT(ABT_timer_start(m_timer)); }

    /**
     * @brief Stops the timer.
     */
    void stop() { TL_TIMER_ASSERT(ABT_timer_stop(m_timer)); }

    /**
     * @brief Reads the current value of the timer.
     * Returns the time difference in seconds between
     * the start time of timer (when timer::start() was called)
     * and the end time of timer (when timer::stop() was called).
     *
     * The resolution of elapsed time is at least a unit of microsecond.
     */
    double read() const {
        double t;
        TL_TIMER_ASSERT(ABT_timer_read(m_timer, &t));
        return t;
    }

    /**
     * @see timer::read
     */
    explicit operator double() const { return read(); }
};

} // namespace thallium

#undef TL_TIMER_EXCEPTION
#undef TL_TIMER_ASSERT

#endif /* end of include guard */
