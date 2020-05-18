/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/async_response.hpp>
#include <thallium/callable_remote_procedure.hpp>
#include <thallium/timeout.hpp>

namespace thallium {

packed_response async_response::wait() {
    hg_return_t ret;
    ret = margo_wait(m_request);
    if(ret == HG_TIMEOUT) {
        throw timeout();
    }
    MARGO_ASSERT(ret, margo_wait);
    if(m_ignore_response)
        return packed_response();
    return packed_response(m_handle, m_engine);
}

} // namespace thallium
