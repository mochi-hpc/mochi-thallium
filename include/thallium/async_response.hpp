/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ASYNC_RESPONSE_HPP
#define __THALLIUM_ASYNC_RESPONSE_HPP

#include <thallium/margo_exception.hpp>
#include <thallium/packed_response.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/timeout.hpp>
#include <utility>
#include <vector>

namespace thallium {

class callable_remote_procedure;

namespace detail {
    struct engine_impl;
}

/**
 * @brief async_response objects are created by sending an
 * RPC in a non-blocking way. They can be used to wait for
 * the actual response.
 */
class async_response {
    friend class callable_remote_procedure;

  private:
    margo_request                      m_request;
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t                        m_handle;
    bool                               m_ignore_response;

    /**
     * @brief Constructor. Made private since async_response
     * objects are created by callable_remote_procedure only.
     *
     * @param req Margo request to wait on.
     * @param e Engine associated with the RPC.
     * @param c callable_remote_procedure that created the async_response.
     * @param ignore_resp whether response should be ignored.
     */
    async_response(margo_request req, const std::weak_ptr<detail::engine_impl>& e,
                   hg_handle_t handle, bool ignore_resp)
    : m_request(req)
    , m_engine_impl(e)
    , m_handle(handle)
    , m_ignore_response(ignore_resp) {
        margo_ref_incr(handle);
    }

  public:
    /**
     * @brief Copy constructor is deleted.
     */
    async_response(const async_response& other) = delete;

    /**
     * @brief Move-constructor.
     *
     * @param other async_response to move from.
     */
    async_response(async_response&& other)
    : m_request(other.m_request)
    , m_engine_impl(std::move(other.m_engine_impl))
    , m_handle(other.m_handle)
    , m_ignore_response(other.m_ignore_response) {
        other.m_request = MARGO_REQUEST_NULL;
        other.m_handle  = HG_HANDLE_NULL;
    }

    /**
     * @brief Copy-assignment operator is deleted.
     */
    async_response& operator=(const async_response& other) = delete;

    /**
     * @brief Move-assignment operator. Will invalidate
     * the moved-from object.
     */
    async_response& operator=(async_response&& other) {
        if(this == &other)
            return *this;
        if(m_handle != HG_HANDLE_NULL)
            margo_destroy(m_handle);
        m_request         = other.m_request;
        m_engine_impl     = std::move(other.m_engine_impl);
        m_handle          = other.m_handle;
        m_ignore_response = other.m_ignore_response;
        other.m_request   = MARGO_REQUEST_NULL;
        other.m_handle    = HG_HANDLE_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~async_response() {
        if(m_handle != HG_HANDLE_NULL)
            margo_destroy(m_handle);
    }

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

    /**
     * @brief Waits for any of the provided async_response to complete,
     * and return a packed_response. The completed iterator will be set to point
     * to the async_response that completed. This method may throw a timeout if
     * any of the requests timed out, or other exceptions if an error happens.
     * Even if an exception is thrown, the completed iterator will be correctly
     * set to point to the async_response in cause.
     *
     * @tparam Iterator Iterator type (e.g.
     * std::vector<async_response>::iterator)
     * @param begin Begin iterator
     * @param end End iterator
     *
     * @return a packed_response.
     */
    template <typename Iterator>
    static packed_response wait_any(const Iterator& begin, const Iterator& end,
                                    Iterator& completed) {
        std::vector<margo_request> reqs;
        size_t                     count = std::distance(begin, end);
        reqs.reserve(count);
        for(auto it = begin; it != end; it++) {
            reqs.push_back(it->m_request);
        }
        completed         = begin;
        size_t      index = 0;
        hg_return_t ret   = margo_wait_any(count, reqs.data(), &index);
        std::advance(completed, index);
        if(ret == HG_TIMEOUT) {
            throw timeout();
        }
        MARGO_ASSERT(ret, margo_wait_any);
        if(completed->m_ignore_response) {
            return packed_response();
        }
        return packed_response(completed->m_handle, completed->m_engine_impl);
    }
};

} // namespace thallium

#endif
