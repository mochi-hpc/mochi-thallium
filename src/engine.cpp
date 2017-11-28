/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <string>
#include <margo.h>
#include <thallium/remote_procedure.hpp>
#include <thallium/engine.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/bulk.hpp>

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

bulk engine::expose(const std::vector<std::pair<void*,size_t>>& segments, bulk_mode flag) {
    hg_bulk_t handle;
    hg_uint32_t count = segments.size();
    std::vector<void*> buf_ptrs(count);
    std::vector<hg_size_t> buf_sizes(count);
    for(unsigned i=0; i < segments.size(); i++) {
        buf_ptrs[i]  = segments[i].first;
        buf_sizes[i] = segments[i].second;
    }
    hg_return_t ret = margo_bulk_create(m_mid, count, &buf_ptrs[0], &buf_sizes[0], (hg_uint32_t)flag, &handle);
    // TODO throw an exception if ret != HG_SUCCESS
    return bulk(*this, handle, true);
}

}

