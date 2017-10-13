#ifndef __THALLIUM_ENDPOINT_HPP
#define __THALLIUM_ENDPOINT_HPP

#include <cstdint>
#include <string>
#include <margo.h>

namespace thallium {

class margo_engine;

class endpoint {

	friend class margo_engine;
	friend class callable_remote_procedure;

private:

	margo_engine& m_margo;
	hg_addr_t     m_addr;

	endpoint(margo_engine& m, hg_addr_t addr)
	: m_margo(m), m_addr(addr) {}

public:

	endpoint(const endpoint& other);

	endpoint(endpoint&& other)
	: m_margo(other.m_margo), m_addr(other.m_addr) {
		other.m_addr = HG_ADDR_NULL;
	}

	endpoint& operator=(const endpoint& other);

	endpoint& operator=(endpoint&& other);
	
	~endpoint();

	operator std::string() const; 
};

}

#endif
