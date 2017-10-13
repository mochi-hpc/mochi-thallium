#include <string>
#include <margo.h>
#include <thallium/remote_procedure.hpp>
#include <thallium/margo_engine.hpp>
#include <thallium/endpoint.hpp>

namespace thallium {

endpoint margo_engine::lookup(const std::string& address) const {

	hg_addr_t addr;
	margo_addr_lookup(m_mid, address.c_str(), &addr);
	// TODO throw exception if address is not known
	return endpoint(const_cast<margo_engine&>(*this), addr);
}

endpoint margo_engine::self() const {
	hg_addr_t self_addr;
	margo_addr_self(m_mid, &self_addr);
	// TODO throw an exception if this call fails
	return endpoint(const_cast<margo_engine&>(*this), self_addr);
}

}

