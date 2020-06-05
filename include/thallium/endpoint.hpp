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
    class engine_impl;
}

class engine;
class request;
class remote_bulk;

/**
 * @brief endpoint objects represent an address to which
 * RPC can be sent. They are created using engine::lookup().
 */
class endpoint {
    friend class engine;
    friend class request;
    friend class callable_remote_procedure;
    friend class remote_bulk;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_addr_t m_addr;

    endpoint(std::weak_ptr<detail::engine_impl> e, hg_addr_t addr)
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
    endpoint()
    : m_engine_impl()
    , m_addr(HG_ADDR_NULL) {}

    /**
     * @brief Copy constructor.
     */
    endpoint(const endpoint& other);

    /**
     * @brief Move constructor.
     */
    endpoint(endpoint&& other)
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
    bool is_null() const { return m_addr == HG_ADDR_NULL; }

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

#endif
