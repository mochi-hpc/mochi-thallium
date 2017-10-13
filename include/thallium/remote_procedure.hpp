#ifndef __THALLIUM_REMOTE_PROCEDURE_HPP
#define __THALLIUM_REMOTE_PROCEDURE_HPP

#include <margo.h>

namespace thallium {

class margo_engine;
class endpoint;
class callable_remote_procedure;

class remote_procedure {

	friend class margo_engine;

private:
	hg_id_t m_id;

	remote_procedure(hg_id_t id)
	: m_id(id) {}

public:

	remote_procedure(const remote_procedure& other)            = default;
	remote_procedure(remote_procedure&& other)                 = default;
	remote_procedure& operator=(const remote_procedure& other) = default;
	remote_procedure& operator=(remote_procedure&& other)      = default;
	~remote_procedure()                                        = default;

	callable_remote_procedure on(const endpoint& ep) const;

	callable_remote_procedure operator,(const endpoint& ep) const;
};

}

#endif
