/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ENDPOINT_HPP
#define __THALLIUM_ENDPOINT_HPP

#include <memory>
#include <cstdint>
#include <margo.h>
#include <string>
#include <thallium/margo_exception.hpp>

namespace thallium {
class endpoint;
}

template <typename S> S& operator<<(S& s, const thallium::endpoint& e);

namespace thallium {

namespace detail {
    struct engine_impl;
}

class engine;
template<typename ... CtxArg> class request_with_context;
class remote_bulk;

/**
 * @brief endpoint objects represent an address to which
 * RPC can be sent. They are created using engine::lookup().
 */
class endpoint {
    friend class engine;
    template<typename ... CtxArg> friend class request_with_context;
    template<typename ... CtxArg> friend class callable_remote_procedure_with_context;
    friend class remote_bulk;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_addr_t m_addr;

    endpoint(std::weak_ptr<detail::engine_impl> e, hg_addr_t addr) noexcept
    : m_engine_impl(std::move(e))
    , m_addr(addr) {}

  public:
    /**
     * @brief Constructor. Made private since endpoint instances
     * can only be created using engine::lookup.
     *
     * @param e Engine that created the endpoint.
     * @param addr Mercury address.
     */
    endpoint(const engine& e, hg_addr_t addr, bool take_ownership = true);

    /**
     * @brief Default constructor defined so that endpoints can
     * be member of other objects and assigned later.
     */
    endpoint() noexcept
    : m_engine_impl()
    , m_addr(HG_ADDR_NULL) {}

    /**
     * @brief Copy constructor.
     */
    endpoint(const endpoint& other);

    /**
     * @brief Move constructor.
     */
    endpoint(endpoint&& other) noexcept
    : m_engine_impl(other.m_engine_impl)
    , m_addr(other.m_addr) {
        other.m_engine_impl.reset();
        other.m_addr = HG_ADDR_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    endpoint& operator=(const endpoint& other);

    /**
     * @brief Move-assignment operator.
     */
    endpoint& operator=(endpoint&& other);

    /**
     * @brief Destructor.
     */
    virtual ~endpoint();

    /**
     * @brief Creates a string representation of the endpoint's address.
     *
     * @return A string representation of the endpoint's address.
     */
    operator std::string() const;

    /**
     * @brief Indicates whether the endpoint is null or not.
     *
     * @return true if the endpoint is a null address.
     */
    bool is_null() const noexcept { return m_addr == HG_ADDR_NULL; }

    /**
     * @brief Returns the underlying Mercury address.
     *
     * @param copy If set to true, a copy of the address will be made.
     * (the user will be responsible for calling margo_addr_free on this
     * address) Otherwise, the hg_addr_t returned is the one managed by
     * the endpoint instance and will be deleted when this enpoint is destroyed.
     *
     * @return The underlying hg_addr_t.
     */
    hg_addr_t get_addr(bool copy = false) const;

    /**
     * @brief Compares two addresses for equality.
     */
    bool operator==(const endpoint& other) const;

    bool operator!=(const endpoint& other) const {
        return !(*this == other);
    }

    /**
     * Hint that the address is no longer valid. This may happen if
     * the peer is no longer responding. This can be used to force removal of
     * the peer address from the list of the peers, before freeing it and
     * reclaim resources.
     */
    void set_remove();

    template <typename S> friend S& ::operator<<(S& s, const endpoint& e);
};

} // namespace thallium

/**
 * @brief Streaming operator for endpoint, converts the endpoint
 * into a string before feeding it to the string operator. This
 * enables, for instance, streaming the endpoint into std::cout
 * for logging without having to explicitely convert it into a
 * string.
 *
 * @tparam S Type of stream.
 * @param s Stream.
 * @param e Endpoint.
 *
 * @return Reference to the provided stream.
 */
template <typename S> S& operator<<(S& s, const thallium::endpoint& e) {
    s << (std::string)e;
    return s;
}

#include <thallium/engine.hpp>
#include <vector>

namespace thallium {

inline endpoint::endpoint(const engine& e, hg_addr_t addr, bool take_ownership)
: m_engine_impl(e.m_impl)
, m_addr(HG_ADDR_NULL) {
    if(!e.m_impl) throw exception("Invalid engine");
    if(take_ownership) {
        m_addr = addr;
    } else {
        hg_return_t ret = margo_addr_dup(e.m_impl->m_mid, addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    }
}

inline endpoint::endpoint(const endpoint& other)
: m_engine_impl(other.m_engine_impl) {
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    if(other.m_addr != HG_ADDR_NULL) {
        hg_return_t ret =
            margo_addr_dup(engine_impl->m_mid, other.m_addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    } else {
        m_addr = HG_ADDR_NULL;
    }
}

inline endpoint& endpoint::operator=(const endpoint& other) {
    hg_return_t ret;
    if(&other == this)
        return *this;
    if(m_addr != HG_ADDR_NULL) {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT(ret, margo_addr_free);
    }
    m_engine_impl = other.m_engine_impl;
    if(other.m_addr != HG_ADDR_NULL) {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        ret = margo_addr_dup(engine_impl->m_mid, other.m_addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    } else {
        m_addr = HG_ADDR_NULL;
    }
    return *this;
}

inline endpoint& endpoint::operator=(endpoint&& other) {
    if(&other == this)
        return *this;
    if(m_addr != HG_ADDR_NULL) {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid engine");
        hg_return_t ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT(ret, margo_addr_free);
    }
    m_engine_impl = other.m_engine_impl;
    m_addr        = other.m_addr;
    other.m_addr  = HG_ADDR_NULL;
    return *this;
}

inline endpoint::~endpoint() {
    if(m_addr != HG_ADDR_NULL) {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) return;
        hg_return_t ret = margo_addr_free(engine_impl->m_mid, m_addr);
        MARGO_ASSERT_TERMINATE(ret, margo_addr_free, -1);
    }
}

inline endpoint::operator std::string() const {
    if(m_addr == HG_ADDR_NULL)
        return std::string();

    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    hg_size_t   size;
    hg_return_t ret =
        margo_addr_to_string(engine_impl->m_mid, NULL, &size, m_addr);
    MARGO_ASSERT(ret, margo_addr_to_string);

    std::vector<char> result(size);

    ret = margo_addr_to_string(engine_impl->m_mid, &result[0], &size, m_addr);
    MARGO_ASSERT(ret, margo_addr_to_string);
    return std::string(result.data());
}

inline hg_addr_t endpoint::get_addr(bool copy) const {
    if(!copy || m_addr == HG_ADDR_NULL)
        return m_addr;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    hg_addr_t   new_addr;
    hg_return_t ret =
        margo_addr_dup(engine_impl->m_mid, m_addr, &new_addr);
    MARGO_ASSERT(ret, margo_addr_dup);
    return new_addr;
}

bool endpoint::operator==(const endpoint& other) const {
    if(is_null() && other.is_null()) return true;
    if(is_null() || other.is_null()) return false;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    return margo_addr_cmp(engine_impl->m_mid, m_addr, other.m_addr) == HG_TRUE;
}

void endpoint::set_remove() {
    if(is_null()) return;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    auto ret = margo_addr_set_remove(engine_impl->m_mid, m_addr);
    MARGO_ASSERT(ret, margo_addr_set_remove);
}

} // namespace thallium

#endif
