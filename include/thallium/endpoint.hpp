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

namespace thallium {

class engine;
class request;
class remote_bulk;

class endpoint {

	friend class engine;
    friend class request;
	friend class callable_remote_procedure;
    friend class remote_bulk;

private:

	engine*   m_engine;
	hg_addr_t m_addr;

	endpoint(engine& e, hg_addr_t addr)
	: m_engine(&e), m_addr(addr) {}

public:

    endpoint()
    : m_engine(nullptr), m_addr(HG_ADDR_NULL) {}

	endpoint(const endpoint& other);

	endpoint(endpoint&& other)
	: m_engine(other.m_engine), m_addr(other.m_addr) {
		other.m_addr = HG_ADDR_NULL;
	}

	endpoint& operator=(const endpoint& other);

	endpoint& operator=(endpoint&& other);
	
	~endpoint();

	operator std::string() const;

    bool is_null() const {
        return m_addr == HG_ADDR_NULL;
    }
};

}

#endif
