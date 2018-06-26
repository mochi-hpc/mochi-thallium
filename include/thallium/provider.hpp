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

    /**
     * @brief Defines an RPC using a member function of the child class.
     * The member function should have a const request& as first parameter
     * and use this request parameter to respond to the client.
     *
     * @tparam S type of the name (e.g. C-like string or std::string)
     * @tparam R return value of the member function
     * @tparam Args Arguments of the member function
     * @param name name of the RPC
     * @param T::*func member function
     */
    template<typename S, typename R, typename ... Args>
    remote_procedure define(S&& name, R(T::*func)(const request&, Args...)) {
        T* self = dynamic_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& r, Args... args) {
            (self->*func)(r, args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id);
    }

private:

    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(S&& name, R(T::*func)(Args...), const std::integral_constant<bool, false>&) {
        T* self = dynamic_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            R r = (self->*func)(args...);
            req.respond(r);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id);
    }

    template<typename S, typename R, typename ... Args>
    remote_procedure define_member(S&& name, R(T::*func)(Args...), const std::integral_constant<bool, true>&) {
        T* self = dynamic_cast<T*>(this);
        std::function<void(const request&, Args...)> fun = [self, func](const request& req, Args... args) {
            (self->*func)(args...);
        };
        return m_engine.define(std::forward<S>(name), fun, m_provider_id).disable_response();
    }

protected:

    /**
     * @brief Defines an RPC from a member function of the child class.
     * This member function doesn't have a const request& paramater, so
     * the RPC will be formed by assuming the return value of the function
     * is what is sent back to the client (nothing is sent back if the
     * return type is void). If the member function returns something but
     * this return value should not be sent back to the client, the caller
     * can pass ignore_return_value() as last argument of define().
     *
     * @tparam S Type of the RPC name (e.g. std::string)
     * @tparam R Return value of the member function.
     * @tparam Args Argument types of the member function.
     * @tparam X Dispatcher type.
     * @param name Name of the RPC.
     * @param T::*func Member function.
     */
    template<typename S, typename R, typename ... Args, typename X = typename std::is_void<R>::type>
    inline remote_procedure define(S&& name, R(T::*func)(Args...), X x = X()) {
        return define_member(std::forward<S>(name), func, x);
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
