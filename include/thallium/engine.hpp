/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ENGINE_HPP
#define __THALLIUM_ENGINE_HPP

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <margo.h>
#include <thallium/tuple_util.hpp>
#include <thallium/function_cast.hpp>
#include <thallium/buffer.hpp>
#include <thallium/request.hpp>
#include <thallium/bulk_mode.hpp>

namespace thallium {

class bulk;
class endpoint;
class remote_bulk;
class remote_procedure;

class engine {

    friend class request;
    friend class bulk;
	friend class endpoint;
    friend class remote_bulk;
    friend class remote_procedure;
	friend class callable_remote_procedure;

private:

    using rpc_t = std::function<void(const request&, const buffer&)>;

	margo_instance_id                  m_mid;
    bool                               m_is_server;
    std::unordered_map<hg_id_t, rpc_t> m_rpcs;

    struct rpc_callback_data {
        engine* m_engine;
        void*   m_function;
    };

    static void free_rpc_callback_data(void* data) {
        rpc_callback_data* cb_data = (rpc_callback_data*)data;
        delete cb_data;
    }

	template<typename F, bool disable_response>
	static void rpc_handler_ult(hg_handle_t handle) {
		using G = std::remove_reference_t<F>;
		const struct hg_info* info = margo_get_info(handle);
		margo_instance_id mid = margo_hg_handle_get_instance(handle);
		void* data = margo_registered_data(mid, info->id);
        auto cb_data  = static_cast<rpc_callback_data*>(data);
		auto f = function_cast<G>(cb_data->m_function);
		request req(*(cb_data->m_engine), handle, disable_response);
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

	template<typename ... Args>
	remote_procedure define(const std::string& name, 
        const std::function<void(const request&, Args...)>& fun);

    template<typename ... Args>
    remote_procedure define(const std::string& name, void (*f)(const request&, Args...));

	endpoint lookup(const std::string& address) const;

    bulk expose(const std::vector<std::pair<void*,size_t>>& segments, bulk_mode flag);

	operator std::string() const;
};

} // namespace thallium

#include <thallium/remote_procedure.hpp>
#include <thallium/proc_buffer.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>

namespace thallium {

template<typename ... Args>
remote_procedure engine::define(const std::string& name, 
        const std::function<void(const request&, Args...)>& fun) {
    // TODO throw an exception if the following call fails

    hg_id_t id = margo_register_name(m_mid, name.c_str(),
                    process_buffer,
                    process_buffer,
                    rpc_callback<rpc_t, false>);

    m_rpcs[id] = [fun,this](const request& r, const buffer& b) {
        std::function<void(Args...)> l = [&fun, &r](Args&&... args) {
            fun(r, std::forward<Args>(args)...);
        };
        std::tuple<std::decay_t<Args>...> iargs;
        if(sizeof...(Args) > 0) {
            buffer_input_archive iarch(b, *this);
            iarch & iargs;
        }
        apply_function_to_tuple(l,iargs);
    };

    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine   = this;
    cb_data->m_function = void_cast(&m_rpcs[id]);

    margo_register_data(m_mid, id, (void*)cb_data, free_rpc_callback_data);
    
    return remote_procedure(*this, id);
}

template<typename ... Args>
remote_procedure engine::define(const std::string& name, void (*f)(const request&, Args...)) {
    return define(name, std::function<void(const request&,Args...)>(f));
}

}

#endif
