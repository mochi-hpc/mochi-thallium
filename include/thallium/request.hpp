/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REQUEST_HPP
#define __THALLIUM_REQUEST_HPP

#include <margo.h>
#include <thallium/margo_exception.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/serialize.hpp>

namespace thallium {

class engine;
class endpoint;
namespace detail {
    struct engine_impl;
}

/**
 * @brief A request object is created whenever a server
 * receives an RPC. The object is passed as first argument to
 * the function associated with the RPC. The request allows
 * one to get information from the caller and to respond to
 * the RPC.
 */
class request {
    friend class engine;
    friend hg_return_t thallium_generic_rpc(hg_handle_t handle);

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t                        m_handle;
    bool                               m_disable_response;

    /**
     * @brief Constructor. Made private since request are only created
     * by the engine within RPC callbacks.
     *
     * @param e engine object that created the request.
     * @param h handle of the RPC that was received.
     * @param disable_resp whether responses are disabled.
     */
    request(const std::weak_ptr<detail::engine_impl>& e, hg_handle_t h, bool disable_resp)
    : m_engine_impl(e)
    , m_handle(h)
    , m_disable_response(disable_resp) {
        margo_ref_incr(m_handle);
    }

  public:
    /**
     * @brief Copy constructor.
     */
    request(const request& other)
    : m_engine_impl(other.m_engine_impl)
    , m_handle(other.m_handle)
    , m_disable_response(other.m_disable_response) {
        hg_return_t ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
    }

    /**
     * @brief Move constructor.
     */
    request(request&& other)
    : m_engine_impl(std::move(other.m_engine_impl))
    , m_handle(other.m_handle)
    , m_disable_response(other.m_disable_response) {
        other.m_handle = HG_HANDLE_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    request& operator=(const request& other) {
        if(m_handle == other.m_handle)
            return *this;
        hg_return_t ret;
        ret = margo_destroy(m_handle);
        MARGO_ASSERT(ret, margo_destroy);
        m_engine_impl      = other.m_engine_impl;
        m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
        ret                = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    request& operator=(request&& other) {
        if(m_handle == other.m_handle)
            return *this;
        margo_destroy(m_handle);
        m_engine_impl      = other.m_engine_impl;
        m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
        other.m_handle     = HG_HANDLE_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~request() {
        hg_return_t ret = margo_destroy(m_handle);
        MARGO_ASSERT_TERMINATE(ret, margo_destroy, -1);
    }

    /**
     * @brief Responds to the sender of the RPC.
     * Serializes the series of arguments provided and
     * send the resulting buffer to the sender.
     *
     * @tparam T Types of parameters to serialize.
     * @param t Parameters to serialize.
     */
    template <typename T1, typename... T>
    void respond(T1&& t1, T&&... t) const {
        if(m_disable_response) {
            throw exception(
                "Calling respond from an RPC that has disabled responses");
        }
        if(m_handle != HG_HANDLE_NULL) {
            auto args = std::make_tuple<const T1&, const T&...>(t1, t...);
            meta_proc_fn mproc = [this, &args](hg_proc_t proc) {
                return proc_object(proc, args, m_engine_impl);
            };
            hg_return_t ret = margo_respond(m_handle, &mproc);
            MARGO_ASSERT(ret, margo_respond);
        } else {
            throw exception("In request::respond : null internal hg_handle_t");
        }
    }

    void respond() const {
        if(m_disable_response) {
            throw exception(
                "Calling respond from an RPC that has disabled responses");
        }
        if(m_handle != HG_HANDLE_NULL) {
            meta_proc_fn mproc = proc_void_object;
            hg_return_t  ret   = margo_respond(m_handle, &mproc);
            MARGO_ASSERT(ret, margo_respond);
        } else {
            throw exception("In request::respond : null internal hg_handle_t");
        }
    }

    /**
     * @brief Get the endpoint corresponding to the sender of the RPC.
     *
     * @return endpoint corresponding to the sender of the RPC.
     */
    endpoint get_endpoint() const;
};

} // namespace thallium

#endif
