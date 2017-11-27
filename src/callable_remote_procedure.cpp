/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */

#include <thallium/callable_remote_procedure.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>

namespace thallium {

callable_remote_procedure::callable_remote_procedure(hg_id_t id, const endpoint& ep, bool ignore_resp) {
    m_ignore_response = ignore_resp;
	// TODO throw exception if this call fails
	margo_create(ep.m_engine.m_mid, ep.m_addr, id, &m_handle);
}

}
