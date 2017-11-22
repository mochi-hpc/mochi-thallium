#ifndef __THALLIUM_ENGINE_HPP
#define __THALLIUM_ENGINE_HPP

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <margo.h>
#include <thallium/function_cast.hpp>
#include <thallium/buffer.hpp>
#include <thallium/request.hpp>

namespace thallium {

class endpoint;
class remote_procedure;

class engine {

	friend class endpoint;
    friend class remote_procedure;
	friend class callable_remote_procedure;

private:

	margo_instance_id                  m_mid;
    bool                               m_is_server;

	template<typename F, bool disable_response>
	static void rpc_handler_ult(hg_handle_t handle) {
		using G = std::remove_reference_t<F>;
		const struct hg_info* info = margo_get_info(handle);
		margo_instance_id mid = margo_hg_handle_get_instance(handle);
		void* data = margo_registered_data(mid, info->id);
		auto f = function_cast<G>(data);
		request req(handle, disable_response);
		buffer input;
		margo_get_input(handle, &input);
		(*f)(req, input);
        margo_free_input(handle, &input);
	}

    template<typename F, bool disable_response>
    static hg_return_t rpc_callback(hg_handle_t handle) {
        int ret;
        ABT_pool pool;
        margo_instance_id mid;
        const struct hg_info *hgi = margo_get_info(handle);
        mid = margo_hg_handle_get_instance(handle);
        if(mid == MARGO_INSTANCE_NULL) {
            return HG_OTHER_ERROR;
        }
        ret = margo_lookup_mplex(mid, hgi->id, hgi->target_id, &pool);
        if(ret != HG_SUCCESS) {
            return HG_INVALID_PARAM;
        }
        ret = ABT_thread_create(pool, (void (*)(void *)) rpc_handler_ult<F,disable_response>, 
                handle, ABT_THREAD_ATTR_NULL, NULL);
        if(ret != 0) {
            return HG_NOMEM_ERROR;
        }
        return HG_SUCCESS;
    }

public:

	engine(const std::string& addr, int mode, 
	              bool use_progress_thread = false,
	              std::int32_t rpc_thread_count = 0) {

        m_is_server = (mode == MARGO_SERVER_MODE);

		m_mid = margo_init(addr.c_str(), mode,
				use_progress_thread ? 1 : 0,
				rpc_thread_count);
		// TODO throw exception if m_mid is null
	}

	engine(const engine& other)            = delete;
	engine(engine&& other)                 = delete;
	engine& operator=(engine&& other)      = delete;
	engine& operator=(const engine& other) = delete;

	~engine() {
        if(m_is_server) {
            // TODO throw an exception if following call fails
            margo_wait_for_finalize(m_mid);
        }
	}

	void finalize() {
		// TODO throw an exception if the following call fails
		margo_finalize(m_mid);
	}

	endpoint self() const;

	remote_procedure define(const std::string& name);

	template<typename F>
	remote_procedure define(const std::string& name, F&& fun);

	endpoint lookup(const std::string& address) const;

	operator std::string() const;
};

} // namespace thallium

#include <thallium/remote_procedure.hpp>
#include <thallium/proc_buffer.hpp>

namespace thallium {

template<typename F>
remote_procedure engine::define(const std::string& name, F&& fun) {
    // TODO throw an exception if the following call fails
    hg_id_t id = margo_register_name(m_mid, name.c_str(),
                    process_buffer,
                    process_buffer,
                    rpc_callback<decltype(fun), false>);
    margo_register_data(m_mid, id, void_cast(&fun), nullptr);
    
    return remote_procedure(*this, id);
}

}

#endif
