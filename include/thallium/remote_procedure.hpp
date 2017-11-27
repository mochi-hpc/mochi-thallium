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
class callable_remote_procedure;

class remote_procedure {

	friend class engine;

private:
    engine& m_engine;
	hg_id_t m_id;
    bool    m_ignore_response;

	remote_procedure(engine& e, hg_id_t id); 

public:

	remote_procedure(const remote_procedure& other)            = default;
	remote_procedure(remote_procedure&& other)                 = default;
	remote_procedure& operator=(const remote_procedure& other) = default;
	remote_procedure& operator=(remote_procedure&& other)      = default;
	~remote_procedure()                                        = default;

	callable_remote_procedure on(const endpoint& ep) const;
    
    remote_procedure& ignore_response();

};

}

#endif
