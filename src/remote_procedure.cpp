#include <thallium/remote_procedure.hpp>
#include <thallium/callable_remote_procedure.hpp>
#include <thallium/engine.hpp>

namespace thallium {

remote_procedure::remote_procedure(engine& e, hg_id_t id) 
: m_engine(e), m_id(id), m_ignore_response(false) { }

callable_remote_procedure remote_procedure::on(const endpoint& ep) const {
	return callable_remote_procedure(m_id, ep, m_ignore_response);
}

remote_procedure& remote_procedure::ignore_response() {
    m_ignore_response = true;
    margo_registered_disable_response(m_engine.m_mid, m_id, HG_TRUE);
    return *this;
}

}
