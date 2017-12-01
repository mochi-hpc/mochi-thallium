/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ENDPOINT_HPP
#define __THALLIUM_ENDPOINT_HPP

#include <cstdint>
#include <string>
#include <margo.h>
#include <thallium/margo_exception.hpp>

namespace thallium {

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

	engine*   m_engine;
	hg_addr_t m_addr;

    /**
     * @brief Constructor. Made private since endpoint instances
     * can only be created using engine::lookup.
     *
     * @param e Engine that created the endpoint.
     * @param addr Mercury address.
     */
	endpoint(engine& e, hg_addr_t addr)
	: m_engine(&e), m_addr(addr) {}

public:

    /**
     * @brief Default constructor defined so that endpoints can
     * be member of other objects and assigned later.
     */
    endpoint()
    : m_engine(nullptr), m_addr(HG_ADDR_NULL) {}

    /**
     * @brief Copy constructor.
     */
	endpoint(const endpoint& other);

    /**
     * @brief Move constructor.
     */
	endpoint(endpoint&& other)
	: m_engine(other.m_engine), m_addr(other.m_addr) {
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
	~endpoint() throw(margo_exception);

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
    bool is_null() const {
        return m_addr == HG_ADDR_NULL;
    }
};

}

#endif
