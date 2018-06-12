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
class callable_remote_procedure;

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
    engine*  m_engine;
	hg_id_t  m_id;
    bool     m_ignore_response;

    /**
     * @brief Constructor. Made private because remote_procedure
     * objects are created only by engine::define().
     *
     * @param e Engine object that created the remote_procedure.
     * @param id Mercury RPC id.
     */
	remote_procedure(engine& e, hg_id_t id); 

public:

    /**
     * @brief Copy-constructor is default.
     */
	remote_procedure(const remote_procedure& other)            = default;

    /**
     * @brief Move-constructor is default.
     */
	remote_procedure(remote_procedure&& other)                 = default;

    /**
     * @brief Copy-assignment operator is default.
     */
	remote_procedure& operator=(const remote_procedure& other) = default;

    /**
     * @brief Move-assignment operator is default.
     */
	remote_procedure& operator=(remote_procedure&& other)      = default;

    /**
     * @brief Destructor is default.
     */
	~remote_procedure()                                        = default;

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
    remote_procedure& ignore_response();

};

}

#endif
