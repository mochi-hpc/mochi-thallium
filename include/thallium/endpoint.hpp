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
#include <thallium/margo_instance_ref.hpp>

namespace thallium {
class endpoint;
}

template <typename S> S& operator<<(S& s, const thallium::endpoint& e);

namespace thallium {

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
    margo_instance_ref m_mid;
    hg_addr_t          m_addr = HG_ADDR_NULL;

  public:
    /**
     * @brief Constructor. Made private since endpoint instances
     * can only be created using engine::lookup.
     *
     * @param e Engine that created the endpoint.
     * @param addr Mercury address.
     */
    endpoint(margo_instance_ref mid, hg_addr_t addr, bool take_ownership = true)
    : m_mid{std::move(mid)}
    , m_addr(addr) {
        MARGO_INSTANCE_MUST_BE_VALID;
        if(!take_ownership && addr != HG_ADDR_NULL) {
            auto ret = margo_addr_dup(m_mid, addr, &m_addr);
            MARGO_ASSERT(ret, margo_addr_dup);
        }
    }

    /**
     * @brief Default constructor defined so that endpoints can
     * be member of other objects and assigned later.
     */
    endpoint() noexcept = default;

    /**
     * @brief Copy constructor.
     */
    endpoint(const endpoint& other)
    : m_mid(other.m_mid)
    , m_addr(HG_ADDR_NULL) {
        MARGO_INSTANCE_MUST_BE_VALID;
        if(other.m_addr == HG_ADDR_NULL) return;
        hg_return_t ret = margo_addr_dup(m_mid, other.m_addr, &m_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
    }

    /**
     * @brief Move constructor.
     */
    endpoint(endpoint&& other) noexcept
    : m_mid(std::move(other.m_mid))
    , m_addr(std::exchange(other.m_addr, HG_ADDR_NULL)) {}

    /**
     * @brief Copy-assignment operator.
     */
    endpoint& operator=(const endpoint& other) {
        hg_return_t ret;
        if(&other == this)
            return *this;
        if(m_addr != HG_ADDR_NULL) {
            ret = margo_addr_free(m_mid, m_addr);
            MARGO_ASSERT(ret, margo_addr_free);
        }
        m_mid = other.m_mid;
        if(other.m_addr != HG_ADDR_NULL) {
            ret = margo_addr_dup(m_mid, other.m_addr, &m_addr);
            MARGO_ASSERT(ret, margo_addr_dup);
        } else {
            m_addr = HG_ADDR_NULL;
        }
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    endpoint& operator=(endpoint&& other) {
        if(&other == this)
            return *this;
        if(m_addr != HG_ADDR_NULL) {
            auto ret = margo_addr_free(m_mid, m_addr);
            MARGO_ASSERT(ret, margo_addr_free);
        }
        m_mid  = std::move(other.m_mid);
        m_addr = std::exchange(other.m_addr, HG_ADDR_NULL);
        return *this;
    }

    /**
     * @brief Destructor.
     */
    virtual ~endpoint() {
        if(m_addr != HG_ADDR_NULL) {
            auto ret = margo_addr_free(m_mid, m_addr);
            MARGO_ASSERT_TERMINATE(ret, margo_addr_free);
        }
    }

    /**
     * @brief Returns the engine that created this endpoint.
     */
    engine get_engine() const;

    /**
     * @brief Creates a string representation of the endpoint's address.
     *
     * @return A string representation of the endpoint's address.
     */
    operator std::string() const {
        std::vector<char> result;
        if(m_addr == HG_ADDR_NULL)
            return std::string{};
        hg_size_t size;
        auto ret = margo_addr_to_string(m_mid, nullptr, &size, m_addr);
        MARGO_ASSERT(ret, margo_addr_to_string);
        result.resize(size);
        ret = margo_addr_to_string(m_mid, const_cast<char*>(result.data()), &size, m_addr);
        MARGO_ASSERT(ret, margo_addr_to_string);
        return std::string{result.data()};
    }

    /**
     * @brief Indicates whether the endpoint is null or not.
     *
     * @return true if the endpoint is a null address.
     */
    bool is_null() const noexcept { return m_addr == HG_ADDR_NULL; }

    /**
     * @brief Returns whether the endpoint is valid.
     */
    operator bool() const noexcept { return !is_null(); }

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
    hg_addr_t get_addr(bool copy = false) const {
        if(!copy || m_addr == HG_ADDR_NULL)
            return m_addr;
        hg_addr_t new_addr;
        auto ret = margo_addr_dup(m_mid, m_addr, &new_addr);
        MARGO_ASSERT(ret, margo_addr_dup);
        return new_addr;
    }

    /**
     * @brief Compares two addresses for equality.
     */
    bool operator==(const endpoint& other) const {
        if(is_null() && other.is_null()) return true;
        if(is_null() || other.is_null()) return false;
        if(m_mid != other.m_mid) return false;
        return margo_addr_cmp(m_mid, m_addr, other.m_addr) == HG_TRUE;
    }

    /**
     * @brief Compares two addresses for equality.
     */
    bool operator!=(const endpoint& other) const {
        return !(*this == other);
    }

    /**
     * Hint that the address is no longer valid. This may happen if
     * the peer is no longer responding. This can be used to force removal of
     * the peer address from the list of the peers, before freeing it and
     * reclaim resources.
     */
    void set_remove() {
        if(is_null()) return;
        auto ret = margo_addr_set_remove(m_mid, m_addr);
        MARGO_ASSERT(ret, margo_addr_set_remove);
    }

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

namespace thallium {

inline engine endpoint::get_engine() const {
    return engine{m_mid};
};

} // namespace thallium

#endif
