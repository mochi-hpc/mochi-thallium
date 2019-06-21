/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/timeout.hpp>
#include <thallium/async_response.hpp>
#include <thallium/callable_remote_procedure.hpp>

namespace thallium {

packed_response async_response::wait() {
    hg_return_t ret;
    ret = margo_wait(m_request);
    if(ret == HG_TIMEOUT) {
        throw timeout();
    }
    MARGO_ASSERT(ret, margo_wait);
    buffer output;
    if(m_ignore_response) return packed_response(std::move(output), *m_engine);
    ret = margo_get_output(m_handle, &output);
    MARGO_ASSERT(ret, margo_get_output);
    ret = margo_free_output(m_handle, &output); // won't do anything on a buffer type
    MARGO_ASSERT(ret, margo_free_output);
    return packed_response(std::move(output), *m_engine);
}

}
