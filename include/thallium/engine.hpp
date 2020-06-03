/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_ENGINE_HPP
#define __THALLIUM_ENGINE_HPP

#include <algorithm>
#include <atomic>
#include <functional>
#include <iostream>
#include <list>
#include <margo.h>
#include <string>
#include <thallium/bulk_mode.hpp>
#include <thallium/config.hpp>
#include <thallium/function_cast.hpp>
#include <thallium/pool.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/request.hpp>
#include <thallium/tuple_util.hpp>
#include <unordered_map>
#include <vector>

#define THALLIUM_SERVER_MODE MARGO_SERVER_MODE
#define THALLIUM_CLIENT_MODE MARGO_CLIENT_MODE

namespace thallium {

class bulk;
class endpoint;
class remote_bulk;
class remote_procedure;
template <typename T> class provider;

DECLARE_MARGO_RPC_HANDLER(thallium_generic_rpc);
hg_return_t thallium_generic_rpc(hg_handle_t handle);

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
    template <typename T> friend class provider;

    friend hg_return_t thallium_generic_rpc(hg_handle_t handle);

  private:
    using rpc_t = std::function<void(const request&)>;

    margo_instance_id                  m_mid;
    bool                               m_is_server;
    bool                               m_owns_mid;
    std::atomic<bool>                  m_finalize_called;
    hg_context_t*                      m_hg_context = nullptr;
    hg_class_t*                        m_hg_class   = nullptr;
    std::list<std::pair<intptr_t, std::function<void(void)>>>
        m_prefinalize_callbacks;
    std::list<std::pair<intptr_t, std::function<void(void)>>>
        m_finalize_callbacks;

    /**
     * @brief Encapsulation of some data needed by RPC callbacks
     * (namely, the initiating thallium engine and the function to call)
     */
    struct rpc_callback_data {
        engine* m_engine;
        rpc_t*  m_function;
    };

    /**
     * @brief Function to call to free the data registered with an RPC.
     *
     * @param data pointer to the data to free (instance of rpc_callback_data).
     */
    static void free_rpc_callback_data(void* data) {
        rpc_callback_data* cb_data = (rpc_callback_data*)data;
        delete cb_data->m_function;
        delete cb_data;
    }

    static void on_finalize_cb(void* arg) {
        engine* e            = static_cast<engine*>(arg);
        e->m_finalize_called = true;
        while(!(e->m_finalize_callbacks.empty())) {
            auto cb = e->m_finalize_callbacks.front();
            e->m_finalize_callbacks.pop_front();
            cb.second();
        }
    }

    static void on_prefinalize_cb(void* arg) {
        engine* e = static_cast<engine*>(arg);
        while(!(e->m_prefinalize_callbacks.empty())) {
            auto cb = e->m_prefinalize_callbacks.front();
            e->m_prefinalize_callbacks.pop_front();
            cb.second();
        }
    }

  public:
    /**
     * @brief Constructor.
     *
     * @param addr address of this instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     * @param use_progress_thread whether to use a dedicated ES to drive
     * progress.
     * @param rpc_thread_count number of threads to use for servicing RPCs.
     * Use -1 to indicate that RPCs should be serviced in the progress ES.
     */
    engine(const std::string& addr, int mode, bool use_progress_thread = false,
           std::int32_t rpc_thread_count = 0) {
        m_is_server       = (mode == THALLIUM_SERVER_MODE);
        m_finalize_called = false;
        m_mid = margo_init(addr.c_str(), mode, use_progress_thread ? 1 : 0,
                           rpc_thread_count);
        // XXX throw an exception if m_mid not initialized
        m_owns_mid = true;
        margo_push_prefinalize_callback(m_mid, &engine::on_prefinalize_cb,
                                        static_cast<void*>(this));
        margo_push_finalize_callback(m_mid, &engine::on_finalize_cb,
                                     static_cast<void*>(this));
    }

    engine(const std::string& addr, int mode, const pool& progress_pool,
           const pool& default_handler_pool) {
        m_is_server       = (mode == THALLIUM_SERVER_MODE);
        m_finalize_called = false;
        m_owns_mid        = true;

        m_hg_class = HG_Init(addr.c_str(), mode);
        // if(!hg_class); // XXX throw exception

        m_hg_context = HG_Context_create(m_hg_class);
        // if(!hg_context); // XXX throw exception

        m_mid =
            margo_init_pool(progress_pool.native_handle(),
                            default_handler_pool.native_handle(), m_hg_context);
        // XXX throw an exception if m_mid not initialized
        margo_push_prefinalize_callback(m_mid, &engine::on_prefinalize_cb,
                                        static_cast<void*>(this));
        margo_push_finalize_callback(m_mid, &engine::on_finalize_cb,
                                     static_cast<void*>(this));
    }

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     */
    [[deprecated]] engine(margo_instance_id mid, int mode) {
        m_mid       = mid;
        m_is_server = (mode == THALLIUM_SERVER_MODE);
        m_owns_mid  = false;
        margo_push_prefinalize_callback(m_mid, &engine::on_prefinalize_cb,
                                        static_cast<void*>(this));
        margo_push_finalize_callback(m_mid, &engine::on_finalize_cb,
                                     static_cast<void*>(this));
    }

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     */
    engine(margo_instance_id mid) {
        m_mid       = mid;
        m_owns_mid  = false;
        m_is_server = margo_is_listening(mid);
        margo_push_prefinalize_callback(m_mid, &engine::on_prefinalize_cb,
                                        static_cast<void*>(this));
        margo_push_finalize_callback(m_mid, &engine::on_finalize_cb,
                                     static_cast<void*>(this));
    }

    /**
     * @brief Copy-constructor is deleted.
     */
    engine(const engine& other) = delete;

    /**
     * @brief Move-constructor is deleted.
     */
    engine(engine&& other) = delete;

    /**
     * @brief Move-assignment operator is deleted.
     */
    engine& operator=(engine&& other) = delete;

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
    margo_instance_id get_margo_instance() const { return m_mid; }

    /**
     * @brief Finalize the engine. Can be called by any thread.
     */
    void finalize() { margo_finalize(m_mid); }

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
    template <typename A1, typename... Args>
    remote_procedure
    define(const std::string&                                      name,
           const std::function<void(const request&, A1, Args...)>& fun,
           uint16_t provider_id = 0, const pool& p = pool());

    remote_procedure define(const std::string&                         name,
                            const std::function<void(const request&)>& fun,
                            uint16_t provider_id = 0, const pool& p = pool());

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
    template <typename... Args>
    remote_procedure define(const std::string& name,
                            void (*f)(const request&, Args...),
                            uint16_t provider_id = 0, const pool& p = pool());

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
     * @param flag indicates whether the bulk is read-write, read-only or
     * write-only.
     *
     * @return a bulk object representing the memory exposed for RDMA.
     */
    bulk expose(const std::vector<std::pair<void*, size_t>>& segments,
                bulk_mode                                    flag);

    /**
     * @brief Creates a bulk object from an hg_bulk_t handle. The user
     * is still responsible for calling margo_bulk_free or HG_Bulk_free
     * on the original handle (this function will increment the hg_bulk_t's
     * internal reference counter).
     *
     * @param blk Bulk handle.
     * @param is_local Whether the bulk handle refers to memory that is local.
     *
     * @return a bulk object representing the memory exposed for RDMA.
     */
    bulk wrap(hg_bulk_t blk, bool is_local);

    /**
     * @brief Pushes a pre-finalization callback into the engine. This callback
     * will be called when margo_finalize is called (e.g. through
     * engine::finalize()), before the Mercury progress loop is terminated.
     *
     * @tparam F type of callback. Must have a operator() defined.
     * @param f callback.
     */
    template <typename F> void push_prefinalize_callback(F&& f) {
        m_prefinalize_callbacks.emplace_back(0, std::forward<F>(f));
    }

    /**
     * @brief Same as push_prefinalize_callback(F&& f) but takes an object whose
     * address will be used to identify the callback (e.g. a provider).
     *
     * @tparam T Type of object used to identify the callback.
     * @tparam F Callback type.
     * @param owner Pointer to the object owning the callback.
     * @param f Callback.
     */
    template <typename T, typename F>
    void push_prefinalize_callback(const T* owner, F&& f) {
        m_prefinalize_callbacks.emplace_back(reinterpret_cast<intptr_t>(owner),
                                             std::forward<F>(f));
    }

    /**
     * @brief Pops the most recently pushed pre-finalization callback and
     * returns it. If no finalization callback are present, this function
     * returns a null std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_prefinalize_callback() {
        auto it = std::find_if(m_prefinalize_callbacks.rbegin(),
                               m_prefinalize_callbacks.rend(),
                               [](const auto& p) { return p.first == 0; });
        if(it != m_prefinalize_callbacks.rend()) {
            auto cb = std::move(it->second);
            m_prefinalize_callbacks.erase(std::next(it).base());
            return cb;
        }
        return std::function<void(void)>();
    }

    /**
     * @brief Pops the most recently pushed pre-finalization callback pushed for
     * a given owner.
     *
     * @tparam T Type of owner.
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    template <typename T>
    std::function<void(void)> pop_prefinalize_callback(const T* owner) {
        auto it = std::find_if(
            m_prefinalize_callbacks.rbegin(), m_prefinalize_callbacks.rend(),
            [owner](const auto& p) {
                return p.first == reinterpret_cast<intptr_t>(owner);
            });
        if(it != m_prefinalize_callbacks.rend()) {
            auto cb = std::move(it->second);
            m_prefinalize_callbacks.erase(std::next(it).base());
            return cb;
        }
        return std::function<void(void)>();
    }

    template <typename F>
    [[deprecated("Use push_finalize_callback")]] void on_finalize(F&& f) {
        m_finalize_callbacks.emplace_back(0, std::forward<F>(f));
    }

    template <typename T, typename F>
    [[deprecated("Use push_finalize_callback")]] void
    on_finalize(const T& owner, F&& f) {
        m_finalize_callbacks.emplace_back(reinterpret_cast<intptr_t>(&owner),
                                          std::forward<F>(f));
    }

    /**
     * @brief Pushes a finalization callback into the engine. This callback will
     * be called when margo_finalize is called (e.g. through
     * engine::finalize()).
     *
     * @tparam F type of callback. Must have a operator() defined.
     * @param f callback.
     */
    template <typename F> void push_finalize_callback(F&& f) {
        m_finalize_callbacks.emplace_back(0, std::forward<F>(f));
    }

    /**
     * @brief Same as push_finalize_callback(F&& f) but takes an object whose
     * address will be used to identify the callback (e.g. a provider).
     *
     * @tparam T Type of object used to identify the callback.
     * @tparam F Callback type.
     * @param owner Pointer to the object owning the callback.
     * @param f Callback.
     */
    template <typename T, typename F>
    void push_finalize_callback(const T* owner, F&& f) {
        m_finalize_callbacks.emplace_back(reinterpret_cast<intptr_t>(owner),
                                          std::forward<F>(f));
    }

    /**
     * @brief Pops the most recently pushed finalization callback and returns
     * it. If no finalization callback are present, this function returns a null
     * std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_finalize_callback() {
        auto it = std::find_if(m_finalize_callbacks.rbegin(),
                               m_finalize_callbacks.rend(),
                               [](const auto& p) { return p.first == 0; });
        if(it != m_finalize_callbacks.rend()) {
            auto cb = std::move(it->second);
            m_finalize_callbacks.erase(std::next(it).base());
            return cb;
        }
        return std::function<void(void)>();
    }

    /**
     * @brief Pops the most recently pushed finalization callback pushed for a
     * given owner.
     *
     * @tparam T Type of owner.
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    template <typename T>
    std::function<void(void)> pop_finalize_callback(const T* owner) {
        auto it = std::find_if(
            m_finalize_callbacks.rbegin(), m_finalize_callbacks.rend(),
            [owner](const auto& p) {
                return p.first == reinterpret_cast<intptr_t>(owner);
            });
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

#include <thallium/proc_object.hpp>
#include <thallium/remote_procedure.hpp>
#include <thallium/serialization/proc_input_archive.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/stl/tuple.hpp>

namespace thallium {

template <typename T1, typename... Tn>
remote_procedure
engine::define(const std::string&                                    name,
               const std::function<void(const request&, T1, Tn...)>& fun,
               uint16_t provider_id, const pool& p) {
    hg_id_t id = MARGO_REGISTER_PROVIDER(
        m_mid, name.c_str(), meta_serialization, meta_serialization,
        thallium_generic_rpc, provider_id, p.native_handle());

    auto rpc_callback = new rpc_t(
        [fun, this](const request& r) {
            std::function<void(T1, Tn...)> call_function =
                [&fun, &r](const T1& a1, const Tn&... args) {
                    fun(r, a1, args...);
                };
                std::tuple<typename std::decay<T1>::type,
                   typename std::decay<Tn>::type...> iargs;
            meta_proc_fn mproc = [this, &iargs](hg_proc_t proc) {
                return proc_object(proc, iargs, this);
            };
            hg_return_t ret = margo_get_input(r.m_handle, &mproc);
            if(ret != HG_SUCCESS)
                return ret;
            ret = margo_free_input(r.m_handle, &mproc);
            if(ret != HG_SUCCESS)
                return ret;
            apply_function_to_tuple(call_function, iargs);
            return HG_SUCCESS;
        }
    );

    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine          = this;
    cb_data->m_function        = rpc_callback;

    hg_return_t ret =
        margo_register_data(m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(*this, id);
}

template <typename... Args>
remote_procedure engine::define(const std::string& name,
                                void (*f)(const request&, Args...),
                                uint16_t provider_id, const pool& p) {
    return define(name, std::function<void(const request&, Args...)>(f),
                  provider_id, p);
}

} // namespace thallium

#endif
