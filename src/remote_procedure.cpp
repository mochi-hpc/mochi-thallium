/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/callable_remote_procedure.hpp>
#include <thallium/engine.hpp>
#include <thallium/provider_handle.hpp>
#include <thallium/remote_procedure.hpp>

namespace thallium {

remote_procedure::remote_procedure(engine& e, hg_id_t id)
: m_engine(&e)
, m_id(id)
, m_ignore_response(false) {}

callable_remote_procedure remote_procedure::on(const endpoint& ep) const {
    return callable_remote_procedure(*m_engine, m_id, ep, m_ignore_response);
}

callable_remote_procedure
remote_procedure::on(const provider_handle& ph) const {
    return callable_remote_procedure(*m_engine, m_id, ph, m_ignore_response,
                                     ph.provider_id());
}

void remote_procedure::deregister() { margo_deregister(m_engine->m_mid, m_id); }

remote_procedure& remote_procedure::disable_response() {
    m_ignore_response = true;
    margo_registered_disable_response(m_engine->m_mid, m_id, HG_TRUE);
    return *this;
}

} // namespace thallium
