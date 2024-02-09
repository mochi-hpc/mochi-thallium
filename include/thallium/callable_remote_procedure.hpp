/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP
#define __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP

#include <chrono>
#include <cstdint>
#include <margo.h>
#include <thallium/async_response.hpp>
#include <thallium/config.hpp>
#include <thallium/margo_exception.hpp>
#include <thallium/packed_data.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/timeout.hpp>
#include <thallium/reference_util.hpp>
#include <tuple>
#include <utility>

namespace thallium {

class engine;
class remote_procedure;
class endpoint;

namespace detail {
    struct engine_impl;
}

/**
 * @brief callable_remote_procedure_with_context objects represent an RPC
 * ready to be called (using the parenthesis operator).
 * It is created from a remote_procedure object using
 * remote_procedure::on(endpoint).
 */
template<typename ... CtxArg>
class callable_remote_procedure_with_context {
    friend class remote_procedure;
    friend class async_response;
    template<typename ... CtxArg2> friend class callable_remote_procedure_with_context;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_handle_t                        m_handle;
    bool                               m_ignore_response;
    uint16_t                           m_provider_id;
    mutable std::tuple<CtxArg...>      m_context;

    callable_remote_procedure_with_context(
            std::weak_ptr<detail::engine_impl> e,
            hg_handle_t handle,
            bool ignore_response,
            uint16_t provider_id,
            std::tuple<CtxArg...>&& context)
    : m_engine_impl(e)
    , m_handle(handle)
    , m_ignore_response(ignore_response)
    , m_provider_id(provider_id)
    , m_context(std::move(context)) {
        if(m_handle != HG_HANDLE_NULL) {
            auto ret = margo_ref_incr(m_handle);
            MARGO_ASSERT(ret, margo_ref_incr);
        }
    }

    /**
     * @brief Constructor. Made private since callable_remote_procedure_with_context can only
     * be created from remote_procedure::on().
     *
     * @param e engine used to create the remote_procedure.
     * @param id id of the RPC to call.
     * @param ep endpoint on which to call the RPC.
     * @param ignore_resp whether the response should be ignored.
     * @param provider_id provider id
     * @param context serialization context
     */
    callable_remote_procedure_with_context(std::weak_ptr<detail::engine_impl> e,
                              hg_id_t id, const endpoint& ep,
                              bool ignore_resp, uint16_t provider_id,
                              const std::tuple<CtxArg...>& context
                              = std::tuple<CtxArg...>());

    /**
     * @brief Sends the RPC to the endpoint (calls margo_forward), passing a
     * buffer in which the arguments have been serialized.
     *
     * @param buf Buffer containing a serialized version of the arguments.
     * @param timeout_ms Timeout in milliseconds. After this timeout, a timeout
     * exception is thrown.
     *
     * @return a packed_data object from which the returned value can be
     * deserialized.
     */
    template <typename... T>
    packed_data<> forward(const std::tuple<T...>& args,
                            double                timeout_ms = -1.0) {
        hg_return_t  ret;
        meta_proc_fn mproc = [this, &args](hg_proc_t proc) {
            return proc_object_encode(proc, const_cast<std::tuple<T...>&>(args),
                               m_engine_impl, m_context);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_forward_timed(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)),
                timeout_ms);
            if(ret == HG_TIMEOUT)
                throw timeout();
            MARGO_ASSERT(ret, margo_provider_iforward);
        } else {
            ret = margo_provider_forward(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)));
            MARGO_ASSERT(ret, margo_provider_forward);
        }
        if(m_ignore_response)
            return packed_data<>();
        return packed_data<>(margo_get_output, margo_free_output, m_handle, m_engine_impl);
    }

    packed_data<> forward(double timeout_ms = -1.0) const {
        hg_return_t  ret;
        meta_proc_fn mproc = [this](hg_proc_t proc) {
            return proc_void_object(proc, m_context);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_forward_timed(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)),
                timeout_ms);
            if(ret == HG_TIMEOUT)
                throw timeout();
            MARGO_ASSERT(ret, margo_provider_forward_timed);
        } else {
            ret = margo_provider_forward(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)));
            MARGO_ASSERT(ret, margo_provider_forward);
        }
        if(m_ignore_response)
            return packed_data<>();
        return packed_data<>(margo_get_output, margo_free_output, m_handle, m_engine_impl);
    }

    /**
     * @brief Sends the RPC to the endpoint (calls margo_iforward), passing a
     * buffer in which the arguments have been serialized. The RPC is sent in a
     * non-blocking manner.
     *
     * @param buf Buffer containing a serialized version of the arguments.
     * @param timeout_ms Optional timeout after which to throw a timeout
     * exception.
     *
     * @return an async_response object that can be waited on.
     *
     * Note: If the request times out, the timeout exception will occure when
     * calling wait() on the async_response.
     */
    template <typename... T>
    async_response iforward(const std::tuple<T...>& args,
                            double                  timeout_ms = -1.0) {
        hg_return_t   ret;
        margo_request req;
        meta_proc_fn  mproc = [this, &args](hg_proc_t proc) {
            return proc_object_encode(proc, const_cast<std::tuple<T...>&>(args),
                                      m_engine_impl, m_context);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_iforward_timed(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)), timeout_ms,
                &req);
            MARGO_ASSERT(ret, margo_provider_iforward_timed);
        } else {
            ret = margo_provider_iforward(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)), &req);
            MARGO_ASSERT(ret, margo_provider_iforward);
        }
        return async_response(req, m_engine_impl, m_handle, m_ignore_response);
    }

    async_response iforward(double timeout_ms = -1.0) const {
        hg_return_t   ret;
        margo_request req;
        meta_proc_fn  mproc = [this](hg_proc_t proc) {
            return proc_void_object(proc, m_context);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_iforward_timed(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)), timeout_ms,
                &req);
            MARGO_ASSERT(ret, margo_provider_iforward_timed);
        } else {
            ret = margo_provider_iforward(
                m_provider_id, m_handle,
                const_cast<void*>(static_cast<const void*>(&mproc)), &req);
            MARGO_ASSERT(ret, margo_provider_iforward);
        }
        return async_response(req, m_engine_impl, m_handle, m_ignore_response);
    }

  public:
    /**
     * @brief Copy-constructor.
     */
    callable_remote_procedure_with_context(
            const callable_remote_procedure_with_context& other)
    : m_engine_impl(other.m_engine_impl)
    , m_handle(other.m_handle)
    , m_ignore_response(other.m_ignore_response)
    , m_provider_id(other.m_provider_id)
    , m_context(other.m_context) {
        hg_return_t ret;
        if(m_handle != HG_HANDLE_NULL) {
            ret = margo_ref_incr(m_handle);
            MARGO_ASSERT(ret, margo_ref_incr);
        }
    }

    /**
     * @brief Move-constructor.
     */
    callable_remote_procedure_with_context(
            callable_remote_procedure_with_context&& other) noexcept
    : m_engine_impl(std::move(other.m_engine_impl))
    , m_handle(other.m_handle)
    , m_ignore_response(other.m_ignore_response)
    , m_provider_id(other.m_provider_id)
    , m_context(std::move(other.m_context)) {
        other.m_handle = HG_HANDLE_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    callable_remote_procedure_with_context&
    operator=(const callable_remote_procedure_with_context& other) {
        hg_return_t ret;
        if(&other == this)
            return *this;
        if(m_handle != HG_HANDLE_NULL) {
            ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
        }
        m_handle          = other.m_handle;
        m_engine_impl     = other.m_engine_impl;
        m_ignore_response = other.m_ignore_response;
        m_provider_id     = other.m_provider_id;
        m_context         = other.m_context;
        ret               = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
        return *this;
    }

    /**
     * @brief Move-assignment operator.
     */
    callable_remote_procedure_with_context&
    operator=(callable_remote_procedure_with_context&& other) {
        if(&other == this)
            return *this;
        if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
        }
        m_handle          = other.m_handle;
        other.m_handle    = HG_HANDLE_NULL;
        m_engine_impl     = std::move(other.m_engine_impl);
        m_ignore_response = other.m_ignore_response;
        m_provider_id     = other.m_provider_id;
        m_context         = std::move(other.m_context);
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~callable_remote_procedure_with_context() {
        if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT_TERMINATE(ret, margo_destroy);
        }
    }

    /**
     * @brief Create a new callable_remote_procedure_with_context with
     * a new context bound to it.
     *
     * @tparam NewCtxArg
     * @param args New context
     */
    template <typename ... NewCtxArg>
    auto with_serialization_context(NewCtxArg&&... args) const {
        return callable_remote_procedure_with_context
            <unwrap_decay_t<NewCtxArg>...>(
                m_engine_impl,
                m_handle,
                m_ignore_response,
                m_provider_id,
                std::make_tuple<NewCtxArg...>(std::forward<NewCtxArg>(args)...));
    }


    /**
     * @brief Operator to call the RPC. Will serialize the arguments
     * in a buffer and send the RPC to the endpoint.
     *
     * @tparam T Types of the parameters.
     * @param t Parameters of the RPC.
     *
     * @return a packed_data object containing the returned value.
     */
    template <typename... T> packed_data<> operator()(const T&... args) {
        return forward(std::make_tuple(std::cref(args)...));
    }

    /**
     * @brief Same as operator() but takes a first parameter representing
     * a timeout (std::duration object). If no response is received from
     * the server before this timeout, the request is cancelled and tl::timeout
     * is thrown.
     *
     * @tparam R
     * @tparam P
     * @tparam T
     * @param t Timeout.
     * @param args Parameters of the RPC.
     *
     * @return a packed_data object containing the returned value.
     */
    template <typename R, typename P, typename... T>
    packed_data<> timed(const std::chrono::duration<R, P>& t,
                          const T&... args) {
        std::chrono::duration<double, std::milli> fp_ms      = t;
        double                                    timeout_ms = fp_ms.count();
        return forward(std::make_tuple(std::cref(args)...), timeout_ms);
    }

    /**
     * @brief Operator to call the RPC without any argument.
     *
     * @return a packed_data object containing the returned value.
     */
    packed_data<> operator()() const { return forward(); }

    /**
     * @brief Same as operator() with only a timeout value.
     *
     * @tparam R
     * @tparam P
     * @param t Timeout.
     *
     * @return a packed_data object containing the returned value.
     */
    template <typename R, typename P>
    packed_data<> timed(const std::chrono::duration<R, P>& t) {
        std::chrono::duration<double, std::milli> fp_ms      = t;
        double                                    timeout_ms = fp_ms.count();
        return forward(timeout_ms);
    }

    /**
     * @brief Issues an RPC in a non-blocking way. Will serialize the arguments
     * in a buffer and send the RPC to the endpoint.
     *
     * @tparam T Types of the parameters.
     * @param t Parameters of the RPC.
     *
     * @return an async_response object that the caller can wait on.
     */
    template <typename... T> async_response async(const T&... args) {
        return iforward(std::make_tuple(std::cref(args)...));
    }

    /**
     * @brief Asynchronous RPC call with a timeout. If the operation times out,
     * the wait() call on the returned async_response object will throw a
     * tl::timeout exception.
     *
     * @tparam R
     * @tparam P
     * @tparam T
     * @param t Timeout.
     * @param args Parameters of the RPC.
     *
     * @return an async_response object that the caller can wait on.
     */
    template <typename R, typename P, typename... T>
    async_response timed_async(const std::chrono::duration<R, P>& t,
                               const T&... args) {
        std::chrono::duration<double, std::milli> fp_ms      = t;
        double                                    timeout_ms = fp_ms.count();
        return iforward(std::make_tuple(std::cref(args)...), timeout_ms);
    }

    /**
     * @brief Non-blocking call to the RPC without any argument.
     *
     * @return an async_response object that the caller can wait on.
     */
    async_response async() { return iforward(); }

    /**
     * @brief Same as async() but with a specified timeout.
     *
     * @tparam R
     * @tparam P
     * @param t Timeout.
     *
     * @return an async_response object that the caller can wait on.
     */
    template <typename R, typename P>
    async_response timed_async(const std::chrono::duration<R, P>& t) {
        std::chrono::duration<double, std::milli> fp_ms      = t;
        double                                    timeout_ms = fp_ms.count();
        return iforward(timeout_ms);
    }
};

using callable_remote_procedure = callable_remote_procedure_with_context<>;

} // namespace thallium

#include <thallium/engine.hpp>
#include <thallium/endpoint.hpp>

namespace thallium {

template<typename ... CtxArg>
inline callable_remote_procedure_with_context<CtxArg...>::
    callable_remote_procedure_with_context(
            std::weak_ptr<detail::engine_impl> e,
            hg_id_t id,
            const endpoint& ep,
            bool     ignore_resp,
            uint16_t provider_id,
            const std::tuple<CtxArg...>& context)
: m_engine_impl(std::move(e))
, m_ignore_response(ignore_resp)
, m_provider_id(provider_id)
, m_context(context) {
    m_ignore_response = ignore_resp;
    auto engine_impl = ep.m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    hg_return_t ret =
        margo_create(engine_impl->m_mid, ep.m_addr, id, &m_handle);
    MARGO_ASSERT(ret, margo_create);
}

} // namespace thallium


#endif
