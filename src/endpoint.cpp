/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>
#include <thallium/margo_exception.hpp>
#include <vector>

namespace thallium {

endpoint::endpoint(const engine& e, hg_addr_t addr, bool take_ownership)
: m_engine_impl(e.m_impl)
, m_addr(HG_ADDR_NULL) {
    // TODO throw if engine is not valid
    if(take_ownership) {
        m_addr = addr;
    } else {
        margo_addr_dup(e.m_impl->m_mid, addr, &m_addr);
    }
}

endpoint::endpoint(const endpoint& other)
: m_engine_impl(other.m_engine_impl) {
    auto engine_impl = m_engine_impl.lock();
    // TODO throw if engine is not valid
    if(other.m_addr != HG_ADDR_NULL) {
        hg_return_t ret =
            margo_addr_dup(engine_impl->m_mid, other.m_addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    } else {
        m_addr = HG_ADDR_NULL;
    }
}

endpoint& endpoint::operator=(const endpoint& other) {
    hg_return_t ret;
    if(&other == this)
        return *this;
    if(m_addr != HG_ADDR_NULL) {
        // TODO throw if engine is not valid
        auto engine_impl = m_engine_impl.lock();
        ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT(ret, margo_addr_free);
    }
    m_engine_impl = other.m_engine_impl;
    if(other.m_addr != HG_ADDR_NULL) {
        // TODO throw if engine is not valid
        auto engine_impl = m_engine_impl.lock();
        ret = margo_addr_dup(engine_impl->m_mid, other.m_addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    } else {
        m_addr = HG_ADDR_NULL;
    }
    return *this;
}

endpoint& endpoint::operator=(endpoint&& other) {
    if(&other == this)
        return *this;
    if(m_addr != HG_ADDR_NULL) {
        // TODO throw if engine is not valid
        auto engine_impl = m_engine_impl.lock();
        hg_return_t ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT(ret, margo_addr_free);
    }
    m_engine_impl = other.m_engine_impl;
    m_addr        = other.m_addr;
    other.m_addr  = HG_ADDR_NULL;
    return *this;
}

endpoint::~endpoint() {
    if(m_addr != HG_ADDR_NULL) {
        // TODO throw if engine is not valid
        auto engine_impl = m_engine_impl.lock();
        hg_return_t ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT_TERMINATE(ret, margo_addr_free, -1);
    }
}

endpoint::operator std::string() const {
    if(m_addr == HG_ADDR_NULL)
        return std::string();

    // TODO throw if engine is not valid
    auto engine_impl = m_engine_impl.lock();
    // TODO throw an exception if one of the following calls fail
    hg_size_t   size;
    hg_return_t ret =
        margo_addr_to_string(engine_impl->m_mid, NULL, &size, m_addr);
    MARGO_ASSERT(ret, margo_addr_to_string);

    std::vector<char> result(size);

    ret = margo_addr_to_string(engine_impl->m_mid, &result[0], &size, m_addr);
    MARGO_ASSERT(ret, margo_addr_to_string);
    return std::string(result.data());
}

hg_addr_t endpoint::get_addr(bool copy) const {
    if(!copy || m_addr == HG_ADDR_NULL)
        return m_addr;
    // TODO throw if engine is not valid
    auto engine_impl = m_engine_impl.lock();
    hg_addr_t   new_addr;
    hg_return_t ret =
        margo_addr_dup(engine_impl->m_mid, m_addr, &new_addr);
    // TODO throw an exception if the call fails
    if(ret != HG_SUCCESS)
        return HG_ADDR_NULL;
    return new_addr;
}

} // namespace thallium
