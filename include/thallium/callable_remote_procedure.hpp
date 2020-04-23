/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP
#define __THALLIUM_CALLABLE_REMOTE_PROCEDURE_HPP

#include <tuple>
#include <cstdint>
#include <utility>
#include <chrono>
#include <margo.h>
#include <thallium/config.hpp>
#include <thallium/timeout.hpp>
#include <thallium/packed_response.hpp>
#include <thallium/async_response.hpp>
#include <thallium/margo_exception.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/stl/vector.hpp>

namespace thallium {

class engine;
class remote_procedure;
class endpoint;

/**
 * @brief callable_remote_procedure objects represent an RPC
 * ready to be called (using the parenthesis operator).
 * It is created from a remote_procedure object using
 * remote_procedure::on(endpoint).
 */
class callable_remote_procedure {

    friend class remote_procedure;
    friend class async_response;

private:
    engine*     m_engine;
    hg_handle_t m_handle;
    bool        m_ignore_response;
    uint16_t    m_provider_id;

    /**
     * @brief Constructor. Made private since callable_remote_procedure can only
     * be created from remote_procedure::on().
     *
     * @param e engine used to create the remote_procedure.
     * @param id id of the RPC to call.
     * @param ep endpoint on which to call the RPC.
     * @param ignore_resp whether the response should be ignored.
     */
    callable_remote_procedure(engine& e, hg_id_t id, const endpoint& ep, 
            bool ignore_resp, uint16_t provider_id=0);

    /**
     * @brief Sends the RPC to the endpoint (calls margo_forward), passing a buffer
     * in which the arguments have been serialized.
     *
     * @param buf Buffer containing a serialized version of the arguments.
     * @param timeout_ms Timeout in milliseconds. After this timeout, a timeout exception is thrown.
     *
     * @return a packed_response object from which the returned value can be deserialized.
     */
    template<typename ... T>
    packed_response forward(const std::tuple<T...>& args, double timeout_ms=-1.0) {
        hg_return_t ret;
        meta_proc_fn mproc = [this, &args](hg_proc_t proc) {
            return proc_object(proc, const_cast<std::tuple<T...>&>(args), m_engine);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_forward_timed(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    timeout_ms);
            if(ret == HG_TIMEOUT)
                throw timeout();
            MARGO_ASSERT(ret, margo_provider_iforward);
        } else {
            ret = margo_provider_forward(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)));
            MARGO_ASSERT(ret, margo_provider_forward);
        }
        if(m_ignore_response) return packed_response();
        return packed_response(m_handle, m_engine);
    }

    packed_response forward(double timeout_ms=-1.0) const {
        hg_return_t ret;
        meta_proc_fn mproc = proc_void_object;
        if(timeout_ms > 0.0) {
            ret = margo_provider_forward_timed(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    timeout_ms);
            if(ret == HG_TIMEOUT)
                throw timeout();
            MARGO_ASSERT(ret, margo_provider_forward_timed);
        } else {
            ret = margo_provider_forward(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)));
            MARGO_ASSERT(ret, margo_provider_forward);
        }
        if(m_ignore_response) return packed_response();
        return packed_response(m_handle, m_engine);
     }

    /**
     * @brief Sends the RPC to the endpoint (calls margo_iforward), passing a buffer
     * in which the arguments have been serialized. The RPC is sent in a non-blocking manner.
     *
     * @param buf Buffer containing a serialized version of the arguments.
     * @param timeout_ms Optional timeout after which to throw a timeout exception.
     *
     * @return an async_response object that can be waited on.
     * 
     * Note: If the request times out, the timeout exception will occure when calling wait()
     * on the async_response.
     */
    template<typename ... T>
    async_response iforward(const std::tuple<T...>& args, double timeout_ms=-1.0) {
        hg_return_t ret;
        margo_request req;
        meta_proc_fn mproc = [this, &args](hg_proc_t proc) {
            return proc_object(proc, const_cast<std::tuple<T...>&>(args), m_engine);
        };
        if(timeout_ms > 0.0) {
            ret = margo_provider_iforward_timed(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    timeout_ms,
                    &req);
            MARGO_ASSERT(ret, margo_provider_iforward_timed);
        } else {
            ret = margo_provider_iforward(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    &req);
            MARGO_ASSERT(ret, margo_provider_iforward);
        }
        return async_response(req, m_engine, m_handle, m_ignore_response);
    }

    async_response iforward(double timeout_ms=-1.0) const {
        hg_return_t ret;
        margo_request req;
        meta_proc_fn mproc = proc_void_object;
        if(timeout_ms > 0.0) {
            ret = margo_provider_iforward_timed(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    timeout_ms,
                    &req);
            MARGO_ASSERT(ret, margo_provider_iforward_timed);
        } else {
            ret = margo_provider_iforward(
                    m_provider_id,
                    m_handle,
                    const_cast<void*>(static_cast<const void*>(&mproc)),
                    &req);
            MARGO_ASSERT(ret, margo_provider_iforward);
        }
        return async_response(req, m_engine, m_handle, m_ignore_response);
    }

public:

    /**
     * @brief Copy-constructor.
     */
    callable_remote_procedure(const callable_remote_procedure& other) {
        hg_return_t ret;
        m_handle = other.m_handle;
        if(m_handle != HG_HANDLE_NULL) {
            ret = margo_ref_incr(m_handle);
            MARGO_ASSERT(ret, margo_ref_incr);
        }
    }

    /**
     * @brief Move-constructor.
     */
    callable_remote_procedure(callable_remote_procedure&& other) {
        m_handle = other.m_handle;
        other.m_handle = HG_HANDLE_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    callable_remote_procedure& operator=(const callable_remote_procedure& other) {
        hg_return_t ret;
        if(&other == this) return *this;
        if(m_handle != HG_HANDLE_NULL) {
            ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
        }
        m_handle = other.m_handle;
        ret = margo_ref_incr(m_handle);
        MARGO_ASSERT(ret, margo_ref_incr);
        return *this;
    }

    
    /**
     * @brief Move-assignment operator.
     */
    callable_remote_procedure& operator=(callable_remote_procedure&& other) {
        if(&other == this) return *this;
        if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT(ret, margo_destroy);
        }
        m_handle = other.m_handle;
        other.m_handle = HG_HANDLE_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~callable_remote_procedure()  {
        if(m_handle != HG_HANDLE_NULL) {
            hg_return_t ret = margo_destroy(m_handle);
            MARGO_ASSERT_TERMINATE(ret, margo_destroy, -1);
        }
    }

    /**
     * @brief Operator to call the RPC. Will serialize the arguments
     * in a buffer and send the RPC to the endpoint.
     *
     * @tparam T Types of the parameters.
     * @param t Parameters of the RPC.
     *
     * @return a packed_response object containing the returned value.
     */
    template<typename ... T>
    packed_response operator()(const T& ... args) {
        return forward(std::make_tuple<const T&...>(args...));
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
     * @return a packed_response object containing the returned value.
     */
    template<typename R, typename P, typename ... T>
    packed_response timed(const std::chrono::duration<R,P>& t, const T& ... args) {
        std::chrono::duration<double, std::milli> fp_ms = t;
        double timeout_ms = fp_ms.count();
        return forward(std::make_tuple<const T&...>(args...), timeout_ms);
    }

    /**
     * @brief Operator to call the RPC without any argument.
     *
     * @return a packed_response object containing the returned value.
     */
    packed_response operator()() const {
        return forward();
    }

    /**
     * @brief Same as operator() with only a timeout value.
     *
     * @tparam R
     * @tparam P
     * @param t Timeout.
     *
     * @return a packed_response object containing the returned value.
     */
    template<typename R, typename P>
    packed_response timed(const std::chrono::duration<R,P>& t) {
        std::chrono::duration<double, std::milli> fp_ms = t;
        double timeout_ms = fp_ms.count();
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
    template<typename ... T>
    async_response async(const T& ... args) {
        return iforward(std::make_tuple<const T&...>(args...));
    }

    /**
     * @brief Asynchronous RPC call with a timeout. If the operation times out,
     * the wait() call on the returned async_response object will throw a tl::timeout
     * exception.
     *
     * @tparam R
     * @tparam P
     * @tparam T
     * @param t Timeout.
     * @param args Parameters of the RPC.
     *
     * @return an async_response object that the caller can wait on.
     */
    template<typename R, typename P, typename ... T>
    async_response timed_async(const std::chrono::duration<R,P>& t, const T& ... args) {
        std::chrono::duration<double, std::milli> fp_ms = t;
        double timeout_ms = fp_ms.count();
        return iforward(std::make_tuple<const T&...>(args...), timeout_ms);
    }

    /**
     * @brief Non-blocking call to the RPC without any argument.
     *
     * @return an async_response object that the caller can wait on.
     */
    async_response async() {
        return iforward();
    }

    /**
     * @brief Same as async() but with a specified timeout.
     *
     * @tparam R
     * @tparam P
     * @param t Timeout.
     *
     * @return an async_response object that the caller can wait on.
     */
    template<typename R, typename P>
    async_response timed_async(const std::chrono::duration<R,P>& t) {
        std::chrono::duration<double, std::milli> fp_ms = t;
        double timeout_ms = fp_ms.count();
        return iforward(timeout_ms);
    }
};

}

#endif
