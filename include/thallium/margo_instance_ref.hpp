/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_MARGO_REF_HPP
#define __THALLIUM_MARGO_REF_HPP

#include <margo.h>
#include <utility>

namespace thallium {

#define MARGO_INSTANCE_MUST_BE_VALID_AUX(func)                                       \
    do {                                                                             \
        if(!m_mid) throw exception{"Trying to call " #func " on an invalid engine"}; \
    } while(0)
#define MARGO_INSTANCE_MUST_BE_VALID MARGO_INSTANCE_MUST_BE_VALID_AUX(__func__)

class margo_instance_ref {

    protected:

    margo_instance_id m_mid = MARGO_INSTANCE_NULL;

    public:

    margo_instance_ref() noexcept = default;

    margo_instance_ref(margo_instance_id mid) noexcept
    : m_mid{mid} {
        margo_instance_ref_incr(m_mid);
    }

    margo_instance_ref(margo_instance_ref&& other) noexcept
    : m_mid{std::exchange(other.m_mid, MARGO_INSTANCE_NULL)} {}

    margo_instance_ref(const margo_instance_ref& other) noexcept
    : margo_instance_ref(other.m_mid) {}

    margo_instance_ref& operator=(margo_instance_ref&& other) noexcept {
        if(this == &other) return *this;
        if(m_mid) margo_instance_release(m_mid);
        m_mid = std::exchange(other.m_mid, MARGO_INSTANCE_NULL);
        return *this;
    }

    margo_instance_ref& operator=(const margo_instance_ref& other) noexcept {
        if(this == &other) return *this;
        if(m_mid) margo_instance_release(m_mid);
        m_mid = other.m_mid;
        if(m_mid) margo_instance_ref_incr(m_mid);
        return *this;
    }

    ~margo_instance_ref() {
        if(m_mid) margo_instance_release(m_mid);
    }

    auto get_margo_instance() const noexcept {
        return m_mid;
    }

    bool operator==(const margo_instance_ref& other) const noexcept {
        return m_mid == other.m_mid;
    }

    bool operator!=(const margo_instance_ref& other) const noexcept {
        return m_mid != other.m_mid;
    }

    operator margo_instance_id() const noexcept {
        return m_mid;
    }

    operator bool() const noexcept {
        return m_mid != MARGO_INSTANCE_NULL;
    }
};

} // namespace thallium

#endif
