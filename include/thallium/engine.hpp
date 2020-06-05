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
class proc_input_archive;
class proc_output_archive;
template <typename T> class provider;

DECLARE_MARGO_RPC_HANDLER(thallium_generic_rpc);
hg_return_t thallium_generic_rpc(hg_handle_t handle);

namespace detail {

    struct engine_impl {
        margo_instance_id                  m_mid;
        bool                               m_is_server;
        bool                               m_owns_mid;
        std::atomic<bool>                  m_finalize_called;
        hg_context_t*                      m_hg_context = nullptr;
        hg_class_t*                        m_hg_class   = nullptr;
    };

}

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
    friend class proc_input_archive;
    friend class proc_output_archive;
    template <typename T> friend class provider;

    friend hg_return_t thallium_generic_rpc(hg_handle_t handle);

  private:
    using rpc_t = std::function<void(const request&)>;
    using finalize_callback_t = std::function<void()>;

    std::shared_ptr<detail::engine_impl> m_impl;

    /**
     * @brief Encapsulation of some data needed by RPC callbacks
     * (namely, the initiating thallium engine and the function to call)
     */
    struct rpc_callback_data {
        std::weak_ptr<detail::engine_impl> m_engine_impl;
        rpc_t                              m_function;
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

    static void on_engine_finalize_cb(void* arg) {
        auto e               = static_cast<detail::engine_impl*>(arg);
        e->m_finalize_called = true;
    }

    static void on_engine_prefinalize_cb(void* arg) {
        auto e = static_cast<detail::engine_impl*>(arg);
        (void)e; // This callback does nothing for now
    }

    static void finalize_callback_wrapper(void* arg) {
        auto cb = static_cast<finalize_callback_t*>(arg);
        (*cb)();
        delete cb;
    }

    engine(std::shared_ptr<detail::engine_impl> impl)
    : m_impl(std::move(impl)) {}

  public:

    /**
     * @brief The default constructor is there to handle the case of
     * declaring and assigning the engine in different places of the code.
     */
    engine() = default;

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
           std::int32_t rpc_thread_count = 0) 
    : m_impl(std::make_shared<detail::engine_impl>()) {
        m_impl->m_is_server       = (mode == THALLIUM_SERVER_MODE);
        m_impl->m_finalize_called = false;
        m_impl->m_mid = margo_init(addr.c_str(), mode, use_progress_thread ? 1 : 0,
                           rpc_thread_count);
        if(!m_impl->m_mid)
            MARGO_THROW(margo_init, "Could not initialize Margo");
        m_impl->m_owns_mid = true;
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
    }

    engine(const std::string& addr, int mode, const pool& progress_pool,
           const pool& default_handler_pool)
    :  m_impl(std::make_shared<detail::engine_impl>()) {
        m_impl->m_is_server       = (mode == THALLIUM_SERVER_MODE);
        m_impl->m_finalize_called = false;
        m_impl->m_owns_mid        = true;

        m_impl->m_hg_class = HG_Init(addr.c_str(), mode);
        if(!m_impl->m_hg_class) {
           throw exception("HG_Init failed in thallium::engine constructor"); 
        }

        m_impl->m_hg_context = HG_Context_create(m_impl->m_hg_class);
        if(!m_impl->m_hg_context) {
            throw exception("HG_Context_create failed in thallium::engine constructor");
        }

        m_impl->m_mid =
            margo_init_pool(progress_pool.native_handle(),
                            default_handler_pool.native_handle(),
                            m_impl->m_hg_context);
        if(!m_impl->m_mid)
            MARGO_THROW(margo_init, "Could not initialize Margo");
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
    }

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     */
    engine(margo_instance_id mid)
    : m_impl(std::make_shared<detail::engine_impl>()) {
        m_impl->m_mid       = mid;
        m_impl->m_owns_mid  = false;
        m_impl->m_is_server = margo_is_listening(mid);
        // an engine initialize with a margo instance is just a wrapper
        // it doesn't need to push prefinalize and finalize callbacks,
        // at least currently.
        /*
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
        */
    }

    /**
     * @brief Copy-constructor.
     */
    engine(const engine& other) = default;

    /**
     * @brief Move-constructor. This method will invalidate
     * the engine that is moved from.
     */
    engine(engine&& other) = default;

    /**
     * @brief Move-assignment operator. This method will invalidate
     * the engine that it moved from.
     */
    engine& operator=(engine&& other) {
        if(m_impl == other.m_impl) return *this;
        if(m_impl) this->~engine();
        m_impl = std::move(other.m_impl);
        return *this;
    }

    /**
     * @brief Copy-assignment operator is deleted.
     */
    engine& operator=(const engine& other) {
        if(m_impl == other.m_impl) return *this;
        if(m_impl) this->~engine();
        m_impl = other.m_impl;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~engine() {
        if(!m_impl) return;
        if(m_impl.use_count() > 1) return;
        if(m_impl->m_owns_mid) {
            if(m_impl->m_is_server) {
                wait_for_finalize();
            } else {
                if(!m_impl->m_finalize_called)
                    finalize();
            }
        }
        if(m_impl->m_hg_context)
            HG_Context_destroy(m_impl->m_hg_context);
        if(m_impl->m_hg_class)
            HG_Finalize(m_impl->m_hg_class);
    }

    /**
     * @brief Get the underlying margo instance. Useful
     * when working in conjunction with C code that need
     * to be initialized with the margo instance.
     *
     * @return The margo instance id.
     */
    margo_instance_id get_margo_instance() const {
        if(!m_impl) throw exception("Invalid engine");
        return m_impl->m_mid;
    }

    /**
     * @brief Finalize the engine. Can be called by any thread.
     */
    void finalize() {
        if(!m_impl) throw exception("Invalid engine");
        margo_finalize(m_impl->m_mid);
    }

    /**
     * @brief Makes the calling thread block until someone calls
     * finalize on this engine. This function will not do anything
     * if finalize was already called.
     */
    void wait_for_finalize() {
        if(!m_impl) throw exception("Invalid engine");
        if(!m_impl->m_finalize_called)
            margo_wait_for_finalize(m_impl->m_mid);
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
        if(!m_impl) throw exception("Invalid engine");
        push_prefinalize_callback(static_cast<const void*>(m_impl.get()), std::forward<F>(f));
    }

    /**
     * @brief Same as push_prefinalize_callback(F&& f) but takes an object whose
     * address will be used to identify the callback (e.g. a provider).
     *
     * @tparam F Callback type.
     * @param owner Pointer to the object owning the callback.
     * @param f Callback.
     */
    template <typename F>
    void push_prefinalize_callback(const void* owner, F&& f) {
        if(!m_impl) throw exception("Invalid engine");
        auto cb = new finalize_callback_t(std::forward<F>(f));
        margo_provider_push_prefinalize_callback(m_impl->m_mid,
            owner,
            finalize_callback_wrapper,
            static_cast<void*>(cb));
    }

    /**
     * @brief Gets the most recently pushed pre-finalization callback and
     * returns it. If no finalization callback are present, this function
     * returns a null std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> top_prefinalize_callback() const {
        if(!m_impl) throw exception("Invalid engine");
        return top_prefinalize_callback(static_cast<const void*>(m_impl.get()));
    }

    /**
     * @brief Gets the most recently pushed pre-finalization callback pushed for
     * a given owner.
     *
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    std::function<void(void)> top_prefinalize_callback(const void* owner) const {
        margo_finalize_callback_t cb = nullptr;
        void* uargs = nullptr;
        if(!m_impl) throw exception("Invalid engine");
        int ret = margo_provider_top_prefinalize_callback(m_impl->m_mid,
            owner,
            &cb,
            &uargs);
        if(ret == 0) return std::function<void(void)>();
        finalize_callback_t *f = static_cast<finalize_callback_t*>(uargs); 
        return *f;
    }

    /**
     * @brief Pops the most recently pushed pre-finalization callback and
     * returns it. If no finalization callback are present, this function
     * returns a null std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_prefinalize_callback() {
        if(!m_impl) throw exception("Invalid engine");
        return pop_prefinalize_callback(static_cast<const void*>(m_impl.get()));
    }

    /**
     * @brief Pops the most recently pushed pre-finalization callback pushed for
     * a given owner.
     *
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_prefinalize_callback(const void* owner) {
        margo_finalize_callback_t cb = nullptr;
        void* uargs = nullptr;
        if(!m_impl) throw exception("Invalid engine");
        int ret = margo_provider_top_prefinalize_callback(m_impl->m_mid,
            owner,
            &cb,
            &uargs);
        if(ret == 0) return std::function<void(void)>();
        finalize_callback_t *f = static_cast<finalize_callback_t*>(uargs);
        finalize_callback_t f_copy = *f;
        delete f;
        if(!m_impl) throw exception("Invalid engine");
        margo_provider_pop_prefinalize_callback(m_impl->m_mid, owner);
        return f_copy;
    }

    template <typename F>
    [[deprecated("Use push_finalize_callback")]] void on_finalize(F&& f) {
        push_finalize_callback(std::forward<F>(f));
    }

    template <typename T, typename F>
    [[deprecated("Use push_finalize_callback")]] void
    on_finalize(const T& owner, F&& f) {
        push_finalize_callback(static_cast<const void*>(&owner), std::forward<F>(f));
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
        if(!m_impl) throw exception("Invalid engine");
        push_finalize_callback(static_cast<const void*>(m_impl.get()), std::forward<F>(f));
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
    template<typename F>
    void push_finalize_callback(const void* owner, F&& f) {
        if(!m_impl) throw exception("Invalid engine");
        auto cb = new finalize_callback_t(std::forward<F>(f));
        margo_provider_push_finalize_callback(m_impl->m_mid,
            owner,
            finalize_callback_wrapper,
            static_cast<void*>(cb));
    }

    /**
     * @brief Getss the most recently pushed finalization callback and returns
     * it. If no finalization callback are present, this function returns a null
     * std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> top_finalize_callback() const {
        if(!m_impl) throw exception("Invalid engine");
        return top_finalize_callback(static_cast<const void*>(m_impl.get()));
    }

    /**
     * @brief Gets the most recently pushed finalization callback pushed for a
     * given owner.
     *
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    std::function<void(void)> top_finalize_callback(const void* owner) const {
        margo_finalize_callback_t cb = nullptr;
        void* uargs = nullptr;
        if(!m_impl) throw exception("Invalid engine");
        int ret = margo_provider_top_finalize_callback(m_impl->m_mid,
            owner,
            &cb,
            &uargs);
        if(ret == 0) return std::function<void(void)>();
        finalize_callback_t *f = static_cast<finalize_callback_t*>(uargs); 
        return *f;
    }

    /**
     * @brief Pops the most recently pushed finalization callback and returns
     * it. If no finalization callback are present, this function returns a null
     * std::function.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_finalize_callback() {
        if(!m_impl) throw exception("Invalid engine");
        return pop_finalize_callback(static_cast<const void*>(m_impl.get()));
    }

    /**
     * @brief Pops the most recently pushed finalization callback pushed for a
     * given owner.
     *
     * @param owner Pointer to the owner.
     *
     * @return finalization callback.
     */
    std::function<void(void)> pop_finalize_callback(const void* owner) {
        margo_finalize_callback_t cb = nullptr;
        void* uargs = nullptr;
        if(!m_impl) throw exception("Invalid engine");
        int ret = margo_provider_top_finalize_callback(m_impl->m_mid,
            owner,
            &cb,
            &uargs);
        if(ret == 0) return std::function<void(void)>();
        finalize_callback_t *f = static_cast<finalize_callback_t*>(uargs); 
        finalize_callback_t f_copy = *f;
        delete f;
        margo_provider_pop_finalize_callback(m_impl->m_mid, owner);
        return f_copy;
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
    if(!m_impl) throw exception("Invalid engine");
    hg_id_t id = MARGO_REGISTER_PROVIDER(
        m_impl->m_mid, name.c_str(), meta_serialization, meta_serialization,
        thallium_generic_rpc, provider_id, p.native_handle());

    std::weak_ptr<detail::engine_impl> w_impl = m_impl;
    auto rpc_callback =
        [fun, w_impl=std::move(w_impl)](const request& r) {
            std::function<void(T1, Tn...)> call_function =
                [&fun, &r](const T1& a1, const Tn&... args) {
                    fun(r, a1, args...);
                };
                std::tuple<typename std::decay<T1>::type,
                   typename std::decay<Tn>::type...> iargs;
            meta_proc_fn mproc = [w_impl, &iargs](hg_proc_t proc) {
                return proc_object(proc, iargs, w_impl);
            };
            hg_return_t ret = margo_get_input(r.m_handle, &mproc);
            if(ret != HG_SUCCESS)
                return ret;
            ret = margo_free_input(r.m_handle, &mproc);
            if(ret != HG_SUCCESS)
                return ret;
            apply_function_to_tuple(call_function, iargs);
            return HG_SUCCESS;
        };

    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine_impl = m_impl;
    cb_data->m_function = std::move(rpc_callback);

    hg_return_t ret =
        margo_register_data(m_impl->m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(m_impl, id);
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
