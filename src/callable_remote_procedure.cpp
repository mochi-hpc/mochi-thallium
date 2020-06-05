/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */

#include <thallium/callable_remote_procedure.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>
#include <thallium/margo_exception.hpp>

namespace thallium {

callable_remote_procedure::callable_remote_procedure(std::weak_ptr<detail::engine_impl> e,
                                                     hg_id_t id,
                                                     const endpoint& ep,
                                                     bool     ignore_resp,
                                                     uint16_t provider_id)
: m_engine_impl(std::move(e))
, m_ignore_response(ignore_resp)
, m_provider_id(provider_id) {
    m_ignore_response = ignore_resp;
    auto engine_impl = ep.m_engine_impl.lock();
    // TODO throw if engine_impl is invalid
    hg_return_t ret =
        margo_create(engine_impl->m_mid, ep.m_addr, id, &m_handle);
    MARGO_ASSERT(ret, margo_create);
}

} // namespace thallium
