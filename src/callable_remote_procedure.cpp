#include <thallium/callable_remote_procedure.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/margo_engine.hpp>

namespace thallium {

callable_remote_procedure::callable_remote_procedure(hg_id_t id, const endpoint& ep) {
	// TODO throw exception if this call fails
	margo_create(ep.m_margo.m_mid, ep.m_addr, id, &m_handle);
}

}
