/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>

namespace thallium {

endpoint::endpoint(const endpoint& other)
: m_engine(other.m_engine) {
	if(other.m_addr != HG_ADDR_NULL) {
		margo_addr_dup(m_engine->m_mid, other.m_addr, &m_addr);
	} else {
		m_addr = HG_ADDR_NULL;
	}
}

endpoint& endpoint::operator=(const endpoint& other) {
	if(&other == this) return *this;
	if(m_addr != HG_ADDR_NULL) {
		margo_addr_free(m_engine->m_mid, m_addr);
	}
    m_engine = other.m_engine;
	if(other.m_addr != HG_ADDR_NULL) {
		margo_addr_dup(m_engine->m_mid, other.m_addr, &m_addr);
	} else {
		m_addr = HG_ADDR_NULL;
	}
	return *this;
}

endpoint& endpoint::operator=(endpoint&& other) {
	if(&other == this) return *this;
	if(m_addr != HG_ADDR_NULL) {
		margo_addr_free(m_engine->m_mid, m_addr);
	}
    m_engine = other.m_engine;
	m_addr = other.m_addr;
	other.m_addr = HG_ADDR_NULL;
	return *this;
}

endpoint::~endpoint() {
	if(m_addr != HG_ADDR_NULL) {
		margo_addr_free(m_engine->m_mid, m_addr);
	}
}

endpoint::operator std::string() const {
    // TODO throw an exception if one of the following calls fail
	if(m_addr == HG_ADDR_NULL) return std::string();

	hg_size_t size;
	margo_addr_to_string(m_engine->m_mid, NULL, &size, m_addr);

	std::string result(size+1,' ');
	size += 1;

	margo_addr_to_string(m_engine->m_mid, &result[0], &size, m_addr);
	return result;
} 

}

