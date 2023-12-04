/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PROVIDER_HPP
#define __THALLIUM_PROVIDER_HPP

#include <atomic>
#include <functional>
#include <iostream>
#include <margo.h>
#include <memory>
#include <string>
#include <thallium/engine.hpp>
#include <thallium/exception.hpp>
#include <thallium/margo_exception.hpp>
#include <unordered_map>

namespace thallium {

typedef std::integral_constant<bool, true> ignore_return_value;

/**
 * @brief The provider class represents an object that
 * exposes its member functions as RPCs. It is a template
 * class meant to be used as base class for other objects.
 * For example:
 * class MyProvider : public thallium::provider<MyProvider> { ... };
 *
 * @tparam T
 */
template <typename T> class provider {
  private:
    std::weak_ptr<detail::engine_impl>  m_engine_impl;
    margo_instance_id                   m_mid;
    uint16_t                            m_provider_id;
    bool                                m_has_identity;

  public:
    provider(const engine& e, uint16_t provider_id, const char* identity = nullptr)
    : m_engine_impl(e.m_impl)
    , m_provider_id(provider_id)
    , m_has_identity(identity != nullptr) {
        if(!e.m_impl) throw exception("Invalid engine");
        m_mid = e.get_margo_instance();
#if MARGO_VERSION_NUM >= 1500
        auto registered_identity = margo_provider_registered_identity(m_mid, provider_id);
        if(m_has_identity) {
            if(registered_identity) {
                throw exception{
                    "[thallium] A (", registered_identity,
                    ") provider with the same ID (",
                    provider_id, ") is already registered"};
            } else {
                auto hret = margo_provider_register_identity(
                        m_mid, m_provider_id, identity);
                MARGO_ASSERT(hret, margo_provider_register_identity);
            }
        }
#endif
    }

    virtual ~provider() {
#if MARGO_VERSION_NUM >= 1500
        if(m_has_identity)
            margo_provider_deregister_identity(m_mid, m_provider_id);
#endif
    }

    /**
     * @brief Copy-constructor.
     */
    provider(const provider& other) = delete;

    /**
     * @brief Move-constructor.
     */
    provider(provider&& other) = delete;

    /**
     * @brief Move-assignment operator.
     */
    provider& operator=(provider&& other) = delete;

    /**
     * @brief Copy-assignment operator.
     */
    provider& operator=(const provider& other) = delete;

    /**
     * @brief Returns the identity of the provider.
     */
    const char* identity() const {
#if MARGO_VERSION_NUM >= 1500
        return margo_provider_registered_identity(
            get_engine().get_margo_instance(), m_provider_id);
#else
        return "<unknown>";
#endif
    }

  protected:
    /**
     * @brief Waits for the engine to finalize.
     */
    [[deprecated("Use the engine's wait_for_finalize method instead")]]
        inline void wait_for_finalize() {
            get_engine().wait_for_finalize();
        }

    /**
     * @brief Finalize the engine.
     */
    [[deprecated("Use the engine's finalize method instead")]]
        inline void finalize() {
            auto engine_impl = m_engine_impl.lock();
            if(!engine_impl) throw exception("Invalid thallium engine state");
            get_engine().finalize();
        }

  private:
    // define_member as RPC for the case return value is NOT void and
    // the first argument is a request. The return value should be ignored,
    // since the user is expected to call req.respond(...).
    template <typename S, typename R, typename A1, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(A1, Args...),
        const std::integral_constant<bool, false>& r_is_void,
        const std::integral_constant<bool, true>&  first_arg_is_request,
        const pool&                                p) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (self->*func)(req, args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value is NOT void
    // and the request is not passed to the function. The return value
    // should be sent using req.respond(...).
    template <typename S, typename R, typename... Args>
    remote_procedure define_member(
        S&&                                        name, R (T::*func)(Args...),
        const std::integral_constant<bool, false>& r_is_void,
        const std::integral_constant<bool, false>& first_arg_is_request,
        const pool&                                p) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                R r = (self->*func)(args...);
                req.respond(r);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_memver as RPC for the case the return value IS void
    // and the first argument is a request. The user is expected to call
    // req.respond(...) himself.
    template <typename S, typename R, typename A1, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(A1, Args...),
        const std::integral_constant<bool, true>& r_is_void,
        const std::integral_constant<bool, true>& first_arg_is_request,
        const pool&                               p = pool()) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (self->*func)(req, args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value IS void
    // and the first argument IS NOT a request. We call disable_response.
    template <typename S, typename R, typename... Args>
    remote_procedure define_member(
        S&&                                        name, R (T::*func)(Args...),
        const std::integral_constant<bool, true>&  r_is_void,
        const std::integral_constant<bool, false>& first_arg_is_request,
        const pool&                                p = pool()) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (void)req;
                (self->*func)(args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p)
            .disable_response();
    }

    // ---
    // the following 4 functions are the same but for "const" member functions
    // ---

    // define_member as RPC for the case return value is NOT void and
    // the first argument is a request. The return value should be ignored,
    // since the user is expected to call req.respond(...).
    template <typename S, typename R, typename A1, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(A1, Args...) const,
        const std::integral_constant<bool, false>& r_is_void,
        const std::integral_constant<bool, true>&  first_arg_is_request,
        const pool&                                p) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (self->*func)(req, args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value is NOT void
    // and the request is not passed to the function. The return value
    // should be sent using req.respond(...).
    template <typename S, typename R, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(Args...) const,
        const std::integral_constant<bool, false>& r_is_void,
        const std::integral_constant<bool, false>& first_arg_is_request,
        const pool&                                p) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                R r = (self->*func)(args...);
                req.respond(r);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_memver as RPC for the case the return value IS void
    // and the first argument is a request. The user is expected to call
    // req.respond(...) himself.
    template <typename S, typename R, typename A1, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(A1, Args...) const,
        const std::integral_constant<bool, true>& r_is_void,
        const std::integral_constant<bool, true>& first_arg_is_request,
        const pool&                               p = pool()) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (self->*func)(req, args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value IS void
    // and the first argument IS NOT a request. We call disable_response.
    template <typename S, typename R, typename... Args>
    remote_procedure define_member(
        S&& name, R (T::*func)(Args...) const,
        const std::integral_constant<bool, true>&  r_is_void,
        const std::integral_constant<bool, false>& first_arg_is_request,
        const pool&                                p = pool()) {
        (void)r_is_void;
        (void)first_arg_is_request;
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun =
            [self, func](const request& req, Args... args) {
                (void)req;
                (self->*func)(args...);
            };
        return get_engine().define(std::forward<S>(name), fun, m_provider_id, p)
            .disable_response();
    }

  protected:
    /**
     * @brief Defines an RPC using a member function of the child class.
     *
     * @tparam S type of the name (e.g. C-like string or std::string)
     * @tparam R return value of the member function
     * @tparam A1 type of the first argument of the member function
     * @tparam Args type of other arguments of the member function
     * @tparam FIRST_ARG_IS_REQUEST automatically deducing whether the first
     * type is a thallium::request
     * @tparam R_IS_VOID automatically deducing whether the return value is void
     * @param name name of the RPC
     * @param T::*func member function
     * @param p Argobots pool
     */
    template <typename S, typename R, typename A1, typename... Args,
              typename FIRST_ARG_IS_REQUEST =
                  typename std::is_same<A1, const request&>::type,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&&         name, R (T::*func)(A1, Args...),
                                   const pool& p,
                                   R_IS_VOID   r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void,
                             FIRST_ARG_IS_REQUEST(), p);
    }

    template <typename S, typename R, typename A1, typename... Args,
              typename FIRST_ARG_IS_REQUEST =
                  typename std::is_same<A1, const request&>::type,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&&       name, R (T::*func)(A1, Args...),
                                   R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void,
                             FIRST_ARG_IS_REQUEST(), pool());
    }

    template <typename S, typename R,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R (T::*func)(), const pool& p,
                                   R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void,
                             first_arg_is_request, p);
    }

    template <typename S, typename R,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&&       name, R (T::*func)(),
                                   R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void,
                             first_arg_is_request, pool());
    }

    // ---
    // the following 4 functions are the same but for "const" member functions
    // ---

    template <typename S, typename R, typename A1, typename... Args,
              typename FIRST_ARG_IS_REQUEST =
                  typename std::is_same<A1, const request&>::type,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R (T::*func)(A1, Args...) const,
                                   const pool& p,
                                   R_IS_VOID   r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void,
                             FIRST_ARG_IS_REQUEST(), p);
    }

    template <typename S, typename R, typename A1, typename... Args,
              typename FIRST_ARG_IS_REQUEST =
                  typename std::is_same<A1, const request&>::type,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R (T::*func)(A1, Args...) const,
                                   R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void,
                             FIRST_ARG_IS_REQUEST(), pool());
    }

    template <typename S, typename R,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&&         name, R (T::*func)() const,
                                   const pool& p,
                                   R_IS_VOID   r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void,
                             first_arg_is_request, p);
    }

    template <typename S, typename R,
              typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&&       name, R (T::*func)() const,
                                   R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void,
                             first_arg_is_request, pool());
    }

  public:
    /**
     * @brief Get the engine associated with this provider.
     *
     * @return The engine associated with this provider.
     */
    engine get_engine() const {
        auto engine_impl = m_engine_impl.lock();
        if(!engine_impl) throw exception("Invalid thallium engine state");
        return engine(engine_impl);
    }

    /**
     * @brief Get the provider id.
     *
     * @return The provider id.
     */
    uint16_t get_provider_id() const { return m_provider_id; }
};

} // namespace thallium

#endif
