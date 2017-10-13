#include <thallium/remote_procedure.hpp>
#include <thallium/callable_remote_procedure.hpp>

namespace thallium {

callable_remote_procedure remote_procedure::on(const endpoint& ep) const {
	return callable_remote_procedure(m_id, ep);
}

callable_remote_procedure remote_procedure::operator,(const endpoint& ep) const {
	return on(ep);
}

}
