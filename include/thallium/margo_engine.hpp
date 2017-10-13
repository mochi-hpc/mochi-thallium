#ifndef __THALLIUM_MARGO_ENGINE_HPP
#define __THALLIUM_MARGO_ENGINE_HPP

#include <iostream>
#include <string>
#include <margo.h>
#include <thallium/function_cast.hpp>
#include <thallium/buffer.hpp>
#include <thallium/request.hpp>

namespace thallium {

class endpoint;
class remote_procedure;

class margo_engine {

	friend class endpoint;
	friend class callable_remote_procedure;

private:

	margo_instance_id m_mid;

	template<typename F>
	static hg_return_t rpc_callback(hg_handle_t handle) {
		using G = std::remove_reference_t<F>;
		const struct hg_info* info = margo_get_info(handle);
		margo_instance_id mid = margo_hg_handle_get_instance(handle);
		void* data = margo_registered_data(mid, info->id);
		auto f = function_cast<G>(data);
		request req(handle);
		(*f)(req);
		return HG_SUCCESS;
	}

public:

	margo_engine(const std::string& addr, int mode, 
	              bool use_progress_thread = false,
	              std::int32_t rpc_thread_count = 0) {

		m_mid = margo_init(addr.c_str(), mode,
				use_progress_thread,
				rpc_thread_count);
		// TODO throw exception if m_mid is null
	}

	margo_engine(const margo_engine& other)            = delete;
	margo_engine(margo_engine&& other)                 = delete;
	margo_engine& operator=(margo_engine&& other)      = delete;
	margo_engine& operator=(const margo_engine& other) = delete;

	~margo_engine() {
		// TODO throw an exception if following call fails
		margo_wait_for_finalize(m_mid);
	}

	void finalize() {
		// TODO throw an exception if the following call fails
		margo_finalize(m_mid);
	}

	endpoint self() const;

	template<typename F>
	remote_procedure define(const std::string& name);

	template<typename F>
	remote_procedure define(const std::string& name, F&& fun);

	endpoint lookup(const std::string& address) const;

	operator std::string() const;
};

} // namespace thallium

#include <thallium/remote_procedure.hpp>

namespace thallium {

template<typename F>
remote_procedure margo_engine::define(const std::string& name) {
    // TODO throw an exception if the following call fails
    hg_id_t id = margo_register_name(m_mid, name.c_str(),
                    nullptr, //proc_vector<char>,
                    nullptr, //proc_vector<char>,
                    nullptr);
    margo_registered_disable_response(m_mid, id, HG_TRUE);
    return remote_procedure(id);
}

template<typename F>
remote_procedure margo_engine::define(const std::string& name, F&& fun) {
    // TODO throw an exception if the following call fails
    hg_id_t id = margo_register_name(m_mid, name.c_str(),
                    nullptr,//serialize<Args...>,
                    nullptr, //proc_vector<char>,
                    rpc_callback<decltype(fun)>);
    margo_register_data(m_mid, id, void_cast(&fun), nullptr);
    margo_registered_disable_response(m_mid, id, HG_TRUE);
    return remote_procedure(id);
}

}

#endif
