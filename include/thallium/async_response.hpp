/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ASYNC_RESPONSE_HPP
#define __THALLIUM_ASYNC_RESPONSE_HPP

#include <thallium/margo_exception.hpp>
#include <thallium/buffer.hpp>
#include <thallium/packed_response.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

namespace thallium {

class callable_remote_procedure;

/**
 * @brief async_response objects are created by sending an
 * RPC in a non-blocking way. They can be used to wait for
 * the actual response.
 */
class async_response {

    friend class callable_remote_procedure;

private:

    margo_request              m_request;
    engine*                    m_engine;
    callable_remote_procedure& m_rpc;
    buffer                     m_buffer;
    bool                       m_ignore_response;

    /**
     * @brief Constructor. Made private since async_response
     * objects are created by callable_remote_procedure only.
     *
     * @param req Margo request to wait on.
     * @param e Engine associated with the RPC.
     * @param c callable_remote_procedure that created the async_response.
     * @param ignore_resp whether response should be ignored.
     */
    async_response(margo_request req, engine& e, callable_remote_procedure& c, bool ignore_resp)
    : m_request(req), m_engine(&e), m_rpc(c), m_ignore_response(ignore_resp) {}

public:

    /**
     * @brief Waits for the async_response to be ready and returns
     * a packed_response when the response has been received.
     *
     * @return a packed_response containing the response.
     */
    packed_response wait();

    /**
     * @brief Tests without blocking if the response has been received.
     *
     * @return true if the response has been received, false otherwise.
     */
    bool received() const {
        int ret;
        int flag;
        ret = margo_test(m_request, &flag);
        MARGO_ASSERT((hg_return_t)ret, margo_test);
        return flag;
    }
};

}

#endif
