/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REMOTE_PROCEDURE_HPP
#define __THALLIUM_REMOTE_PROCEDURE_HPP

#include <margo.h>

namespace thallium {

class engine;
class endpoint;
class provider_handle;
template<typename ... CtxArg> class callable_remote_procedure_with_context;
using callable_remote_procedure = callable_remote_procedure_with_context<>;
namespace detail {
    struct engine_impl;
}

/**
 * @brief remote_procedure objects are produced by
 * engine::define() when defining an RPC.
 * Using remote_procedure::on(endpoint) creates a
 * callable_remote_procedure that can be called with
 * some parameters to send an RPC.
 */
class remote_procedure {
    friend class engine;

  private:
    std::weak_ptr<detail::engine_impl> m_engine_impl;
    hg_id_t                            m_id = 0;
    bool                               m_ignore_response;

    /**
     * @brief Constructor. Made private because remote_procedure
     * objects are created only by engine::define().
     *
     * @param e Engine object that created the remote_procedure.
     * @param id Mercury RPC id.
     */
    remote_procedure(std::weak_ptr<detail::engine_impl> e, hg_id_t id)
    : m_engine_impl(std::move(e))
    , m_id(id)
    , m_ignore_response(false) {}

  public:

    remote_procedure() = default;

    /**
     * @brief Copy-constructor is default.
     */
    remote_procedure(const remote_procedure& other) = default;

    /**
     * @brief Move-constructor is default.
     */
    remote_procedure(remote_procedure&& other) = default;

    /**
     * @brief Copy-assignment operator is default.
     */
    remote_procedure& operator=(const remote_procedure& other) = default;

    /**
     * @brief Move-assignment operator is default.
     */
    remote_procedure& operator=(remote_procedure&& other) = default;

    /**
     * @brief Destructor is default.
     */
    ~remote_procedure() = default;

    /**
     * @brief Creates a callable_remote_procedure by associating the
     * remote_procedure with an endpoint.
     *
     * @param ep endpoint with which to associate the procedure.
     *
     * @return a callable_remote_procedure.
     */
    callable_remote_procedure on(const endpoint& ep) const;

    /**
     * @brief Creates a callable remote_procedure by associating the
     * remote_procedure with a particular provider_handle.
     *
     * @param ph provider_handle with which to associate the procedure.
     *
     * @return a callable_remote_procedure.
     */
    callable_remote_procedure on(const provider_handle& ph) const;

    /**
     * @brief Tell the remote_procedure that it should not expect responses.
     *
     * @return *this
     */
    remote_procedure& disable_response();

    /**
     * @brief Deregisters this RPC from the engine.
     */
    void deregister();

    [[deprecated("use disable_response() instead")]] inline remote_procedure&
    ignore_response() {
        return disable_response();
    }
};

} // namespace thallium

#include <thallium/callable_remote_procedure.hpp>
#include <thallium/engine.hpp>
#include <thallium/provider_handle.hpp>

namespace thallium {

inline callable_remote_procedure remote_procedure::on(const endpoint& ep) const {
    if(m_id == 0)
        throw exception("remote_procedure object isn't initialized");
    return callable_remote_procedure(m_engine_impl, m_id, ep, m_ignore_response, 0);
}

inline callable_remote_procedure
remote_procedure::on(const provider_handle& ph) const {
    if(m_id == 0)
        throw exception("remote_procedure object isn't initialized");
    return callable_remote_procedure(m_engine_impl, m_id, ph, m_ignore_response,
                                     ph.provider_id());
}

inline void remote_procedure::deregister() {
    auto engine_impl = m_engine_impl.lock();
    if(engine_impl)
        margo_deregister(engine_impl->m_mid, m_id);
}

inline remote_procedure& remote_procedure::disable_response() {
    m_ignore_response = true;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("remote_procedure object isn't initialized");
    margo_registered_disable_response(engine_impl->m_mid, m_id, HG_TRUE);
    return *this;
}

} // namespace thallium


#endif
