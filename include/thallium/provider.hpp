/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_PROVIDER_HPP
#define __THALLIUM_PROVIDER_HPP

#include <memory>
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <margo.h>
#include <thallium/engine.hpp>

namespace thallium {

typedef std::integral_constant<bool,true> ignore_return_value;

/**
 * @brief The provider class represents an object that
 * exposes its member functions as RPCs. It is a template
 * class meant to be used as base class for other objects.
 * For example:
 * class MyProvider : public thallium::provider<MyProvider> { ... };
 *
 * @tparam T
 */
template<typename T>
class provider {

private:

    engine&  m_engine;
    uint16_t m_provider_id;

public:

	provider(engine& e, uint16_t provider_id)
    : m_engine(e), m_provider_id(provider_id) {}

    virtual ~provider() {}

    /**
     * @brief Copy-constructor is deleted.
     */
	provider(const provider& other)            = delete;

    /**
     * @brief Move-constructor is deleted.
     */
	provider(provider&& other)                 = delete;
    
    /**
     * @brief Move-assignment operator is deleted.
     */
	provider& operator=(provider&& other)      = delete;

    /**
     * @brief Copy-assignment operator is deleted.
     */
	provider& operator=(const provider& other) = delete;

    protected:
    /**
     * @brief Waits for the engine to finalize.
     */
    inline void wait_for_finalize() {
        m_engine.wait_for_finalize();
    }

    /**
     * @brief Finalize the engine.
     */
    inline void finalize() {
        m_engine.finalize();
    }


private:

    // define_member as RPC for the case return value is NOT void and
    // the first argument is a request. The return value should be ignored,
    // since the user is expected to call req.respond(...).
    template<typename S, typename R, typename A1, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(A1, Args...),
            const std::integral_constant<bool, false>& r_is_void,
            const std::integral_constant<bool, true>& first_arg_is_request,
            const pool& p) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(req, args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value is NOT void
    // and the request is not passed to the function. The return value
    // should be sent using req.respond(...).
    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(Args...),
            const std::integral_constant<bool, false>& r_is_void,
            const std::integral_constant<bool, false>& first_arg_is_request,
            const pool& p) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            R r = (self->*func)(args...);
            req.respond(r);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_memver as RPC for the case the return value IS void
    // and the first argument is a request. The user is expected to call
    // req.respond(...) himself.
    template<typename S, typename R, typename A1, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(A1, Args...),
            const std::integral_constant<bool, true>& r_is_void,
            const std::integral_constant<bool, true>& first_arg_is_request,
            const pool& p = pool()) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(req, args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value IS void
    // and the first argument IS NOT a request. We call disable_response.
    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(Args...),
            const std::integral_constant<bool, true>& r_is_void,
            const std::integral_constant<bool, false>& first_arg_is_request,
            const pool& p = pool()) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p).disable_response();
    }

    // ---
    // the following 4 functions are the same but for "const" member functions
    // ---

    // define_member as RPC for the case return value is NOT void and
    // the first argument is a request. The return value should be ignored,
    // since the user is expected to call req.respond(...).
    template<typename S, typename R, typename A1, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(A1, Args...) const,
            const std::integral_constant<bool, false>& r_is_void,
            const std::integral_constant<bool, true>& first_arg_is_request,
            const pool& p) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(req, args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value is NOT void
    // and the request is not passed to the function. The return value
    // should be sent using req.respond(...).
    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(Args...) const,
            const std::integral_constant<bool, false>& r_is_void,
            const std::integral_constant<bool, false>& first_arg_is_request,
            const pool& p) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            R r = (self->*func)(args...);
            req.respond(r);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_memver as RPC for the case the return value IS void
    // and the first argument is a request. The user is expected to call
    // req.respond(...) himself.
    template<typename S, typename R, typename A1, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(A1, Args...) const,
            const std::integral_constant<bool, true>& r_is_void,
            const std::integral_constant<bool, true>& first_arg_is_request,
            const pool& p = pool()) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(req, args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p);
    }

    // define_member as RPC for the case the return value IS void
    // and the first argument IS NOT a request. We call disable_response.
    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(
            S&& name,
            R(T::*func)(Args...) const,
            const std::integral_constant<bool, true>& r_is_void,
            const std::integral_constant<bool, false>& first_arg_is_request,
            const pool& p = pool()) {
        T* self = static_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id, p).disable_response();
    }
protected:

    /**
     * @brief Defines an RPC using a member function of the child class.
     *
     * @tparam S type of the name (e.g. C-like string or std::string)
     * @tparam R return value of the member function
     * @tparam A1 type of the first argument of the member function
     * @tparam Args type of other arguments of the member function
     * @tparam FIRST_ARG_IS_REQUEST automatically deducing whether the first type is a thallium::request
     * @tparam R_IS_VOID automatically deducing whether the return value is void
     * @param name name of the RPC
     * @param T::*func member function
     * @param p Argobots pool
     */
    template<typename S, typename R, typename A1, typename ... Args, 
             typename FIRST_ARG_IS_REQUEST = typename std::is_same<A1, const request&>::type,
             typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(A1, Args...), const pool& p, R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void, FIRST_ARG_IS_REQUEST(), p);
    }

    template<typename S, typename R, typename A1, typename ... Args, 
             typename FIRST_ARG_IS_REQUEST = typename std::is_same<A1, const request&>::type,
             typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(A1, Args...), R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void, FIRST_ARG_IS_REQUEST(), pool());
    }

    template<typename S, typename R, typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(), const pool& p, R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void, first_arg_is_request, p);
    }

    template<typename S, typename R, typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(), R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void, first_arg_is_request, pool());
    }

    // ---
    // the following 4 functions are the same but for "const" member functions
    // ---

    template<typename S, typename R, typename A1, typename ... Args, 
             typename FIRST_ARG_IS_REQUEST = typename std::is_same<A1, const request&>::type,
             typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(A1, Args...) const, const pool& p, R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void, FIRST_ARG_IS_REQUEST(), p);
    }

    template<typename S, typename R, typename A1, typename ... Args, 
             typename FIRST_ARG_IS_REQUEST = typename std::is_same<A1, const request&>::type,
             typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(A1, Args...) const, R_IS_VOID r_is_void = R_IS_VOID()) {
        return define_member(std::forward<S>(name), func, r_is_void, FIRST_ARG_IS_REQUEST(), pool());
    }

    template<typename S, typename R, typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)() const, const pool& p, R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void, first_arg_is_request, p);
    }

    template<typename S, typename R, typename R_IS_VOID = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)() const, R_IS_VOID r_is_void = R_IS_VOID()) {
        std::integral_constant<bool, false> first_arg_is_request;
        return define_member(std::forward<S>(name), func, r_is_void, first_arg_is_request, pool());
    }
public:

    /**
     * @brief Get the engine associated with this provider.
     *
     * @return The engine associated with this provider.
     */
    const engine& get_engine() const {
        return m_engine;
    }

    /**
     * @brief Get the engine associated with this provider.
     *
     * @return The engine associated with this provider.
     */
    engine& get_engine() {
        return m_engine;
    }

    /**
     * @brief Get the provider id.
     *
     * @return The provider id.
     */
    uint16_t get_provider_id() const {
        return m_provider_id;
    }
};

} // namespace thallium

#endif
