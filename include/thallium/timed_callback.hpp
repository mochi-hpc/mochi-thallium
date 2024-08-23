/*
 * Copyright (c) 2022 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_TIMED_CALLBACK_HPP
#define __THALLIUM_TIMED_CALLBACK_HPP

#include <margo.h>
#include <margo-timer.h>
#include <thallium/exception.hpp>
#include <thallium/margo_instance_ref.hpp>
#include <memory>
#include <functional>

namespace thallium {

class engine;

/**
 * @brief Class returned by engine::schedule.
 */
class timed_callback {

    friend class engine;

    margo_timer_t                          m_timer = MARGO_TIMER_NULL;
    std::unique_ptr<std::function<void()>> m_callback;

    timed_callback(margo_instance_id mid, std::function<void()> fun)
    : m_callback(std::make_unique<std::function<void()>>(std::move(fun))) {
        auto ret = margo_timer_create(mid, &call, m_callback.get(), &m_timer);
        if(ret != 0) throw exception("Could not create timed_callback");
    }

    static void call(void* uargs) {
        auto cb = static_cast<std::function<void()>*>(uargs);
        (*cb)();
    }

    public:

    timed_callback(const timed_callback&) = delete;
    timed_callback(timed_callback&&) = default;
    timed_callback& operator=(const timed_callback&) = delete;

    /**
     * @brief Move-assignment operator.
     *
     * @warning: do not use this method if the callback has been scheduled.
     */
    timed_callback& operator=(timed_callback&& other) {
        if(this == &other) return *this;
        if(m_timer) margo_timer_cancel(m_timer);
        m_timer = std::exchange(other.m_timer, MARGO_TIMER_NULL);
        m_callback = std::move(other.m_callback);
        return *this;
    }

    ~timed_callback() {
        if(m_timer != MARGO_TIMER_NULL)
            margo_timer_destroy(m_timer);
    }

    void start(double timeout_ms) {
        if(0 != margo_timer_start(m_timer, timeout_ms))
            throw exception("Could not start timed_callback: "
                            "timer invalid or already started");
    }

    void cancel() {
        if(0 != margo_timer_cancel(m_timer))
            throw exception("Could not cancel timed_callback: "
                            "timer invalid or not started");
    }
};

} // namespace thallium

#endif /* end of include guard */
