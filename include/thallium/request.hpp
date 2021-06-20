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
#include <thallium/packed_data.hpp>

namespace thallium {

class engine;
class endpoint;
namespace detail {
    struct engine_impl;
}

/**
 * @brief A request_with_context object is created whenever a server
 * receives an RPC. The object is passed as first argument to
 * the function associated with the RPC. The request_with_context allows
 * one to get information from the caller and to respond to
 * the RPC.
 */
template<typename... CtxArg>
class request_with_context {
    friend class engine;
    friend hg_return_t thallium_generic_rpc(hg_handle_t handle);

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t                        m_handle;
    bool                               m_disable_response;
    mutable std::tuple<CtxArg...>      m_context;

    /**
     * @brief Constructor. Made private since request_with_context are only created
     * by the engine within RPC callbacks.
     *
     * @param e engine object that created the request_with_context.
     * @param h handle of the RPC that was received.
     * @param disable_resp whether responses are disabled.
     */
    request_with_context(std::weak_ptr<detail::engine_impl> e,
                    hg_handle_t h,
                    bool disable_resp,
                    std::tuple<CtxArg...>&& context = std::tuple<CtxArg...>())
    : m_engine_impl(std::move(e))
    , m_handle(h)
    , m_disable_response(disable_resp)
    , m_context(std::move(context)) {
        margo_ref_incr(m_handle);
    }

  public:
    /**
     * @brief Copy constructor.
     */
    request_with_context(const request_with_context& other)
    : m_engine_impl(other.m_engine_impl)
    , m_handle(other.m_handle)
    , m_disable_response(other.m_disable_response)
    , m_context(other.context) {
        hg_return_t ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
    }

    /**
     * @brief Move constructor.
     */
    request_with_context(request_with_context&& other) noexcept
    : m_engine_impl(std::move(other.m_engine_impl))
    , m_handle(other.m_handle)
    , m_disable_response(other.m_disable_response)
    , m_context(std::move(other.m_context)) {
        other.m_handle = HG_HANDLE_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    request_with_context& operator=(const request_with_context& other) {
        if(m_handle == other.m_handle)
            return *this;
        hg_return_t ret;
        ret = margo_destroy(m_handle);
        MARGO_ASSERT(ret, margo_destroy);
        m_engine_impl      = other.m_engine_impl;
        m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
        m_context          = other.m_context;
        ret                = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    request_with_context& operator=(request_with_context&& other) noexcept {
        if(m_handle == other.m_handle)
            return *this;
        margo_destroy(m_handle);
        m_engine_impl      = other.m_engine_impl;
        m_handle           = other.m_handle;
        m_disable_response = other.m_disable_response;
        m_context          = std::move(other.m_context);
        other.m_handle     = HG_HANDLE_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~request_with_context() {
        hg_return_t ret = margo_destroy(m_handle);
        MARGO_ASSERT_TERMINATE(ret, margo_destroy, -1);
    }

    /**
     * @brief Get the input of the RPC as a packed_data object.
     */
    auto get_input() const {
        return packed_data<>(
            margo_get_input,
            margo_free_input,
            m_handle,
            m_engine_impl);
    }

    /**
     * @brief Create a new request_with_context object with a new
     * context bound to it for response serialization.
     *
     * @tparam NewCtxArg
     * @param args New context.
     */
    template<typename ... NewCtxArg>
    auto with_serialization_context(NewCtxArg&&... args) const {
        return request_with_context<NewCtxArg...>(
                m_engine_impl,
                m_handle,
                m_disable_response,
                std::make_tuple<NewCtxArg...>(std::forward<NewCtxArg>(args)...));
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
                return proc_object(proc, args, m_engine_impl, m_context);
            };
            hg_return_t ret = margo_respond(m_handle, &mproc);
            MARGO_ASSERT(ret, margo_respond);
        } else {
            throw exception("In request_with_context::respond : null internal hg_handle_t");
        }
    }

    void respond() const {
        if(m_disable_response) {
            throw exception(
                "Calling respond from an RPC that has disabled responses");
        }
        if(m_handle != HG_HANDLE_NULL) {
            meta_proc_fn mproc = [this](hg_proc_t proc) {
                return proc_void_object(proc, m_context);
            };
            hg_return_t  ret   = margo_respond(m_handle, &mproc);
            MARGO_ASSERT(ret, margo_respond);
        } else {
            throw exception("In request_with_context::respond : null internal hg_handle_t");
        }
    }

    /**
     * @brief Get the endpoint corresponding to the sender of the RPC.
     *
     * @return endpoint corresponding to the sender of the RPC.
     */
    endpoint get_endpoint() const;
};

using request = request_with_context<>;

} // namespace thallium

#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>

namespace thallium {

template<typename ... CtxArg>
inline endpoint request_with_context<CtxArg...>::get_endpoint() const {
    const struct hg_info* info = margo_get_info(m_handle);
    hg_addr_t             addr;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    hg_return_t ret = margo_addr_dup(engine_impl->m_mid, info->addr, &addr);
    MARGO_ASSERT(ret, margo_addr_dup);
    return endpoint(m_engine_impl, addr);
}

} // namespace thallium

#endif
