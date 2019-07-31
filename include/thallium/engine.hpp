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
#include <algorithm>
#include <list>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <margo.h>
#include <thallium/config.hpp>
#include <thallium/pool.hpp>
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
template<typename T> class provider;

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
    template<typename T>
    friend class provider;

private:

    using rpc_t = std::function<void(const request&, const buffer&)>;

    margo_instance_id                     m_mid;
    std::unordered_map<hg_id_t, rpc_t>    m_rpcs;
    bool                                  m_is_server;
    bool                                  m_owns_mid;
    std::atomic<bool>                     m_finalize_called;
    hg_context_t*                         m_hg_context = nullptr;
    hg_class_t*                           m_hg_class = nullptr;
    std::list<std::pair<intptr_t, std::function<void(void)>>> m_finalize_callbacks;

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
        using G = typename std::remove_reference<F>::type;
        const struct hg_info* info = margo_get_info(handle);
        THALLIUM_ASSERT_CONDITION(info != nullptr, "margo_get_info returned null");
        margo_instance_id mid = margo_hg_handle_get_instance(handle);
        THALLIUM_ASSERT_CONDITION(mid != 0, "margo_hg_handle_get_instance returned null");
        void* data = margo_registered_data(mid, info->id);
        THALLIUM_ASSERT_CONDITION(data != nullptr, "margo_registered_data returned null");
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
        margo_destroy(handle); // because of margo_ref_incr in rpc_callback
        __margo_internal_decr_pending(mid);
        if(__margo_internal_finalize_requested(mid)) {
            margo_finalize(mid);
        }
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
        mid = margo_hg_handle_get_instance(handle);
        if(mid == MARGO_INSTANCE_NULL) {
            return HG_OTHER_ERROR;
        }
        pool = margo_hg_handle_get_handler_pool(handle);
        __margo_internal_incr_pending(mid);
        margo_ref_incr(handle);
        ret = ABT_thread_create(pool, (void (*)(void *)) rpc_handler_ult<F,disable_response>, 
                handle, ABT_THREAD_ATTR_NULL, NULL);
        if(ret != 0) {
            margo_destroy(handle);
            return HG_NOMEM_ERROR;
        }
        return HG_SUCCESS;
    }

    static void on_finalize_cb(void* arg) {
        engine* e = static_cast<engine*>(arg);
        e->m_finalize_called = true;
        while(!(e->m_finalize_callbacks.empty())) {
            auto& cb = e->m_finalize_callbacks.front();
            cb.second();
            e->m_finalize_callbacks.pop_front();
        }
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
        m_finalize_called = false;
        m_mid = margo_init(addr.c_str(), mode,
                use_progress_thread ? 1 : 0,
                rpc_thread_count);
        // XXX throw an exception if m_mid not initialized
        m_owns_mid = true;
        margo_push_finalize_callback(m_mid,
                &engine::on_finalize_cb, static_cast<void*>(this));
    }

    engine(const std::string& addr, int mode,
            const pool& progress_pool,
            const pool& default_handler_pool) {
        m_is_server = (mode == THALLIUM_SERVER_MODE);
        m_finalize_called = false;
        m_owns_mid = true;

        m_hg_class = HG_Init(addr.c_str(), mode);
        //if(!hg_class); // XXX throw exception

        m_hg_context = HG_Context_create(m_hg_class);
        //if(!hg_context); // XXX throw exception

        m_mid = margo_init_pool(progress_pool.native_handle(), 
                default_handler_pool.native_handle(), m_hg_context);
        // XXX throw an exception if m_mid not initialized
        margo_push_finalize_callback(m_mid,
                &engine::on_finalize_cb, static_cast<void*>(this));
    }

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     */
    [[deprecated]]
    engine(margo_instance_id mid, int mode) {
        m_mid = mid;
        m_is_server = (mode == THALLIUM_SERVER_MODE);
        m_owns_mid = false;
        margo_push_finalize_callback(m_mid,
                &engine::on_finalize_cb, static_cast<void*>(this));
    }

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     */
    engine(margo_instance_id mid) {
        m_mid = mid;
        m_owns_mid = false;
        m_is_server = margo_is_listening(mid);
        margo_push_finalize_callback(m_mid,
                &engine::on_finalize_cb, static_cast<void*>(this));
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
    ~engine() {
        if(m_owns_mid) {
            if(m_is_server) {
                if(!m_finalize_called) 
                    margo_wait_for_finalize(m_mid);
            } else {
                if(!m_finalize_called)
                    finalize();
            }
        }
        if(m_hg_context)
            HG_Context_destroy(m_hg_context);
        if(m_hg_class)
            HG_Finalize(m_hg_class);
    }

    /**
     * @brief Get the underlying margo instance. Useful
     * when working in conjunction with C code that need
     * to be initialized with the margo instance.
     *
     * @return The margo instance id.
     */
    margo_instance_id get_margo_instance() const {
        return m_mid;
    }

    /**
     * @brief Finalize the engine. Can be called by any thread.
     */
    void finalize() {
        margo_finalize(m_mid);
    }

    /**
     * @brief Makes the calling thread block until someone calls
     * finalize on this engine. This function will not do anything
     * if finalize was already called.
     */
    void wait_for_finalize() {
        if(!m_finalize_called)
            margo_wait_for_finalize(m_mid);
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
     * @param provider_id ID of the provider registering this RPC.
     * @param pool Argobots pool to use when receiving this type of RPC
     *
     * @return a remote_procedure object.
     */
    template<typename A1, typename ... Args>
    remote_procedure define(const std::string& name, 
        const std::function<void(const request&, A1, Args...)>& fun,
        uint16_t provider_id=0, const pool& p = pool());

    remote_procedure define(const std::string& name, 
        const std::function<void(const request&)>& fun,
        uint16_t provider_id=0, const pool& p = pool());

    /**
     * @brief Defines an RPC with a name and a function pointer
     * to call when the RPC is received.
     *
     * @tparam Args Types of arguments accepted by the RPC.
     * @param name Name of the RPC.
     * @param f Function to associate with the RPC.
     * @param provider_id ID of the provider registering this RPC.
     * @param pool Argobots pool to use when receiving this type of RPC.
     *
     * @return a remote_procedure object.
     */
    template<typename ... Args>
    remote_procedure define(const std::string& name, void (*f)(const request&, Args...),
            uint16_t provider_id=0, const pool& p = pool());

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

    template<typename F>
    [[deprecated("Use push_finalize_callback")]] void on_finalize(F&& f) {
        m_finalize_callbacks.emplace_back(0,std::forward<F>(f));
    }

    template<typename T, typename F>
    [[deprecated("Use push_finalize_callback")]] void on_finalize(const T& owner, F&& f) {
        m_finalize_callbacks.emplace_back(reinterpret_cast<intptr_t>(&owner), std::forward<F>(f));
    }

    /**
     * @brief Pushes a finalization callback into the engine. This callback will be
     * called when margo_finalize is called (e.g. through engine::finalize()).
     *
     * @tparam F type of callback. Must have a operator() defined.
     * @param f callback.
     */
    template<typename F>
    void push_finalize_callback(F&& f) {
        m_finalize_callbacks.emplace_back(0,std::forward<F>(f));
    }

    /**
     * @brief Same as push_finalize_callback(F&& f) but takes an object whose address will
     * be used to identify the callback (e.g. a provider).
     *
     * @tparam T Type of object used to identify the callback.
     * @tparam F Callback type.
     * @param owner Pointer to the object owning the callback.
     * @param f Callback.
     */
    template<typename T, typename F>
    void push_finalize_callback(const T* owner, F&& f) {
        m_finalize_callbacks.emplace_back(reinterpret_cast<intptr_t>(owner), std::forward<F>(f));
    }

    /**
     * @brief Pops the most recently pushed finalization callback and returns it.
     * If no finalization callback are present, this function returns a null std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_finalize_callback() {
        auto it = std::find_if(m_finalize_callbacks.rbegin(), m_finalize_callbacks.rend(),
                [](const auto& p) { return p.first == 0; });
        if(it != m_finalize_callbacks.rend()) {
            auto cb = std::move(it->second);
            m_finalize_callbacks.erase(std::next(it).base());
            return cb;
        }
        return std::function<void(void)>();
    }

    /**
     * @brief Pops the most recently pushed finalization callback pushed for a given owner.
     *
     * @tparam T Type of owner.
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    template<typename T>
    std::function<void(void)> pop_finalize_callback(const T* owner) {
        auto it = std::find_if(m_finalize_callbacks.rbegin(), m_finalize_callbacks.rend(),
                [owner](const auto& p) { return p.first == reinterpret_cast<intptr_t>(owner); });
        if(it != m_finalize_callbacks.rend()) {
            auto cb = std::move(it->second);
            m_finalize_callbacks.erase(std::next(it).base());
            return cb;
        }
        return std::function<void(void)>();
    }

    /**
     * @brief Shuts down a remote thallium engine. The remote engine
     * should have enabled remote shutdown by calling enable_remote_shutdown().
     *
     * @param ep endpoint of the remote engine.
     */
    void shutdown_remote_engine(const endpoint& ep) const;

    /**
     * @brief Enables this engine to be shutdown remotely.
     */
    void enable_remote_shutdown();
};

} // namespace thallium

#include <thallium/remote_procedure.hpp>
#include <thallium/proc_buffer.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>
#include <thallium/serialization/stl/tuple.hpp>

namespace thallium {

template<typename A1, typename ... Args>
remote_procedure engine::define(const std::string& name, 
        const std::function<void(const request&, A1, Args...)>& fun,
        uint16_t provider_id, const pool& p) {

    hg_id_t id = margo_provider_register_name(m_mid, name.c_str(),
                process_buffer,
                process_buffer,
                rpc_callback<rpc_t, false>,
                provider_id,
                p.native_handle());

    m_rpcs[id] = [fun,this](const request& r, const buffer& b) {
        std::function<void(A1, Args...)> call_function = [&fun, &r](A1&& a1, Args&&... args) {
            fun(r, std::forward<A1>(a1), std::forward<Args>(args)...);
        };
        std::tuple<typename std::decay<A1>::type, typename std::decay<Args>::type...> iargs;
        buffer_input_archive iarch(b, *this);
        iarch(iargs);
        apply_function_to_tuple(call_function, iargs);
    };

    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine   = this;
    cb_data->m_function = void_cast(&m_rpcs[id]);

    hg_return_t ret = margo_register_data(m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(*this, id);
}

template<typename ... Args>
remote_procedure engine::define(
        const std::string& name,
        void (*f)(const request&, Args...),
        uint16_t provider_id, const pool& p) {

    return define(name, std::function<void(const request&,Args...)>(f), provider_id, p);
}

}

#endif
