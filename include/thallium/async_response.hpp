/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ASYNC_RESPONSE_HPP
#define __THALLIUM_ASYNC_RESPONSE_HPP

#include <thallium/margo_instance_ref.hpp>
#include <thallium/margo_exception.hpp>
#include <thallium/packed_data.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/timeout.hpp>
#include <utility>
#include <vector>

namespace thallium {


template<typename ... CtxArg> class callable_remote_procedure_with_context;
using callable_remote_procedure = callable_remote_procedure_with_context<>;

/**
 * @brief async_response objects are created by sending an
 * RPC in a non-blocking way. They can be used to wait for
 * the actual response.
 */
class async_response {
    template<typename ... CtxArg> friend class callable_remote_procedure_with_context;

  private:
    margo_instance_ref m_mid;
    margo_request      m_request = MARGO_REQUEST_NULL;
    hg_handle_t        m_handle  = HG_HANDLE_NULL;
    bool               m_ignore_response = false;

    /**
     * @brief Constructor. Made private since async_response
     * objects are created by callable_remote_procedure only.
     *
     * @param req Margo request to wait on.
     * @param mid Margo instance associated with the RPC.
     * @param c callable_remote_procedure that created the async_response.
     * @param ignore_resp whether response should be ignored.
     */
    async_response(margo_request req, margo_instance_ref mid,
                   hg_handle_t handle, bool ignore_resp) noexcept
    : m_mid(std::move(mid))
    , m_request(req)
    , m_handle(handle)
    , m_ignore_response(ignore_resp) {
        margo_ref_incr(handle);
    }

  public:

    async_response() = default;

    /**
     * @brief Copy constructor is deleted.
     */
    async_response(const async_response& other) = delete;

    /**
     * @brief Move-constructor.
     *
     * @param other async_response to move from.
     */
    async_response(async_response&& other) noexcept
    : m_mid(std::move(other.m_mid))
    , m_request{std::exchange(other.m_request, MARGO_REQUEST_NULL)}
    , m_handle{std::exchange(other.m_handle, HG_HANDLE_NULL)}
    , m_ignore_response(other.m_ignore_response) {}

    /**
     * @brief Copy-assignment operator is deleted.
     */
    async_response& operator=(const async_response& other) = delete;

    /**
     * @brief Move-assignment operator.
     */
    async_response& operator=(async_response&& other) {
        if(&other == this || m_request == other.m_request) return *this;
        if(m_request != MARGO_REQUEST_NULL)
            wait();
        if(m_handle != HG_HANDLE_NULL)
            margo_destroy(m_handle);
        m_mid             = std::move(other.m_mid);
        m_request         = std::exchange(other.m_request, MARGO_REQUEST_NULL);
        m_handle          = std::exchange(other.m_handle, HG_HANDLE_NULL);
        m_ignore_response = other.m_ignore_response;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~async_response() noexcept {
        if(m_request != MARGO_REQUEST_NULL)
            wait();
        if(m_handle != HG_HANDLE_NULL)
            margo_destroy(m_handle);
    }

    /**
     * @brief Waits for the async_response to be ready and returns
     * a packed_data when the response has been received.
     *
     * @return a packed_data containing the response.
     */
    packed_data<> wait() {
        if(m_handle == HG_HANDLE_NULL)
            throw exception("Calling wait on an invalid async_response");
        hg_return_t ret;
        if(m_request != MARGO_REQUEST_NULL) {
            ret = margo_wait(m_request);
            m_request = MARGO_REQUEST_NULL;
            if(ret == HG_TIMEOUT) {
                throw timeout();
            }
            MARGO_ASSERT(ret, margo_wait);
        }
        if(m_ignore_response)
            return packed_data<>();
        return packed_data<>(margo_get_output, margo_free_output, m_handle, m_mid);
    }

    /**
     * @brief Tests without blocking if the response has been received.
     *
     * @return true if the response has been received, false otherwise.
     */
    bool received() const {
        if(m_request == MARGO_REQUEST_NULL)
            return true;
        int ret;
        int flag;
        ret = margo_test(m_request, &flag);
        MARGO_ASSERT((hg_return_t)ret, margo_test);
        return flag;
    }

    /**
     * @brief Waits for any of the provided async_response to complete,
     * and return a packed_data. The completed iterator will be set to point
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
     * @return a packed_data.
     */
    template <typename Iterator>
    static packed_data<> wait_any(const Iterator& begin, const Iterator& end,
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
            return packed_data<>();
        }
        return packed_data<>(margo_get_output, margo_free_output,
                completed->m_handle, completed->m_mid);
    }
};

} // namespace thallium

#endif
