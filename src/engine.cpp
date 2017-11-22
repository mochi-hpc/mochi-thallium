#include <string>
#include <margo.h>
#include <thallium/remote_procedure.hpp>
#include <thallium/engine.hpp>
#include <thallium/endpoint.hpp>

namespace thallium {

endpoint engine::lookup(const std::string& address) const {

	hg_addr_t addr;
	margo_addr_lookup(m_mid, address.c_str(), &addr);
	// TODO throw exception if address is not known
	return endpoint(const_cast<engine&>(*this), addr);
}

endpoint engine::self() const {
	hg_addr_t self_addr;
	margo_addr_self(m_mid, &self_addr);
	// TODO throw an exception if this call fails
	return endpoint(const_cast<engine&>(*this), self_addr);
}

remote_procedure engine::define(const std::string& name) {
    // TODO throw an exception if the following call fails
    hg_id_t id = margo_register_name(m_mid, name.c_str(),
                    process_buffer,
                    process_buffer,
                    nullptr);
    return remote_procedure(*this, id);
}

}

