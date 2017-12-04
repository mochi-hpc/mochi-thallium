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

#define THALLIUM_SERVER_MODE MARGO_SERVER_MODE
#define THALLIUM_CLIENT_MODE MARGO_CLIENT_MODE

namespace thallium {

class bulk;
class endpoint;
class remote_bulk;
class remote_procedure;

/**
 * @brief The engine class is at the core of Thallium,
 * it is the first object to instanciate to start using the
 * Thallium runtime. It initializes Margo and other libraries,
 * and allow users to declare RPCs and bulk objects.
 */
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
    std::unordered_map<hg_id_t, rpc_t> m_rpcs;
    bool                               m_is_server;
    bool                               m_owns_mid;

    /**
     * @brief Encapsulation of some data needed by RPC callbacks
     * (namely, the initiating thallium engine and the function to call)
     */
    struct rpc_callback_data {
        engine* m_engine;
        void*   m_function;
    };

    /**
     * @brief Function to call to free the data registered with an RPC.
     *
     * @param data pointer to the data to free (instance of rpc_callback_data).
     */
    static void free_rpc_callback_data(void* data) {
        rpc_callback_data* cb_data = (rpc_callback_data*)data;
        delete cb_data;
    }

    /**
     * @brief Function run as a ULT when receiving an RPC.
     *
     * @tparam F type of the function to call.
     * @tparam disable_response whether the caller expects a response.
     * @param handle handle of the RPC.
     */
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
		hg_return_t ret;
        ret = margo_get_input(handle, &input);
        MARGO_ASSERT(ret, margo_get_input);
		(*f)(req, input);
        ret = margo_free_input(handle, &input);
        MARGO_ASSERT(ret, margo_free_input);
	}

    /**
     * @brief Callback called when an RPC is received.
     *
     * @tparam F type of the function exposed by the user for this RPC.
     * @tparam disable_response whether the caller expects a response.
     * @param handle handle of the RPC.
     *
     * @return HG_SUCCESS or a Mercury error code.
     */
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

    /**
     * @brief Constructor.
     *
     * @param addr address of this instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     * @param use_progress_thread whether to use a dedicated ES to drive progress.
     * @param rpc_thread_count number of threads to use for servicing RPCs.
     * Use -1 to indicate that RPCs should be serviced in the progress ES.
     */
	engine(const std::string& addr, int mode, 
	              bool use_progress_thread = false,
	              std::int32_t rpc_thread_count = 0) {

        m_is_server = (mode == THALLIUM_SERVER_MODE);

		m_mid = margo_init(addr.c_str(), mode,
				use_progress_thread ? 1 : 0,
				rpc_thread_count);
        // XXX throw an exception if m_mid not initialized
        m_owns_mid = true;
	}

    engine(margo_instance_id mid, int mode) {
        m_mid = mid;
        m_is_server = (mode == THALLIUM_SERVER_MODE);
        m_owns_mid = false;
    }

    /**
     * @brief Copy-constructor is deleted.
     */
	engine(const engine& other)            = delete;

    /**
     * @brief Move-constructor is deleted.
     */
	engine(engine&& other)                 = delete;
    
    /**
     * @brief Move-assignment operator is deleted.
     */
	engine& operator=(engine&& other)      = delete;

    /**
     * @brief Copy-assignment operator is deleted.
     */
	engine& operator=(const engine& other) = delete;


    /**
     * @brief Destructor.
     */
	~engine() throw(margo_exception) {
        if(m_owns_mid) {
            if(m_is_server) {
                margo_wait_for_finalize(m_mid);
            } else {
                margo_finalize(m_mid);
            }
        }
	}


    /**
     * @brief Finalize the engine. Can be called by any thread.
     */
	void finalize() {
		margo_finalize(m_mid);
	}

    /**
     * @brief Creates an endpoint from this engine.
     *
     * @return An endpoint corresponding to this engine.
     */
	endpoint self() const;

    /**
     * @brief Defines an RPC with a name, without providing a
     * function pointer (used on clients).
     *
     * @param name Name of the RPC.
     *
     * @return a remote_procedure object.
     */
	remote_procedure define(const std::string& name);

    /**
     * @brief Defines an RPC with a name and an std::function 
     * representing the RPC.
     *
     * @tparam Args Types of arguments accepted by the RPC.
     * @param name Name of the RPC.
     * @param fun Function to associate with the RPC.
     *
     * @return a remote_procedure object.
     */
	template<typename ... Args>
	remote_procedure define(const std::string& name, 
        const std::function<void(const request&, Args...)>& fun);

    /**
     * @brief Defines an RPC with a name and a function pointer
     * to call when the RPC is received.
     *
     * @tparam Args Types of arguments accepted by the RPC.
     * @param name Name of the RPC.
     * @param f Function to associate with the RPC.
     *
     * @return a remote_procedure object.
     */
    template<typename ... Args>
    remote_procedure define(const std::string& name, void (*f)(const request&, Args...));

    /**
     * @brief Lookup an address and returns an endpoint object
     * to communicate with this address.
     *
     * @param address String representation of the address.
     *
     * @return an endpoint object associated with the given address.
     */
	endpoint lookup(const std::string& address) const;

    /**
     * @brief Exposes a series of memory segments for bulk operations.
     *
     * @param segments vector of <pointer,size> pairs of memory segments.
     * @param flag indicates whether the bulk is read-write, read-only or write-only.
     *
     * @return a bulk object representing the memory exposed for RDMA.
     */
    bulk expose(const std::vector<std::pair<void*,size_t>>& segments, bulk_mode flag);

    /**
     * @brief String representation of the engine's address.
     *
     * @return String representation of the engine's address.
     */
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

    hg_return_t ret = margo_register_data(m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(*this, id);
}

template<typename ... Args>
remote_procedure engine::define(const std::string& name, void (*f)(const request&, Args...)) {
    return define(name, std::function<void(const request&,Args...)>(f));
}

}

#endif
