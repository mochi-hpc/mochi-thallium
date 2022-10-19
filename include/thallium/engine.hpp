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
#include <thallium/margo_exception.hpp>
#include <thallium/config.hpp>
#include <thallium/tuple_util.hpp>
#include <thallium/function_util.hpp>
#include <thallium/logger.hpp>
#include <unordered_map>
#include <vector>

#define THALLIUM_SERVER_MODE MARGO_SERVER_MODE
#define THALLIUM_CLIENT_MODE MARGO_CLIENT_MODE

namespace thallium {

class bulk;
class endpoint;
class remote_bulk;
class remote_procedure;
class timed_callback;
class pool;
template <typename ... CtxArg> class request_with_context;
using request = request_with_context<>;
template <typename ... CtxArg> class proc_input_archive;
template <typename ... CtxArg> class proc_output_archive;
template <typename T> class provider;
class xstream;
class pool;

DECLARE_MARGO_RPC_HANDLER(thallium_generic_rpc)
hg_return_t thallium_generic_rpc(hg_handle_t handle);

namespace detail {

    struct engine_impl {
        std::atomic<margo_instance_id> m_mid;
        bool                           m_owns_mid;
    };

}

/**
 * @brief The engine class is at the core of Thallium,
 * it is the first object to instanciate to start using the
 * Thallium runtime. It initializes Margo and other libraries,
 * and allow users to declare RPCs and bulk objects.
 */
class engine {
    template<typename ... CtxArg> friend class request_with_context;
    friend class bulk;
    friend class endpoint;
    friend class remote_bulk;
    friend class remote_procedure;
    template <typename ... CtxArg> friend class callable_remote_procedure_with_context;
    template <typename ... CtxArg> friend class proc_input_archive;
    template <typename ... CtxArg> friend class proc_output_archive;
    template <typename T> friend class provider;
    friend class timed_callback;

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
    static void free_rpc_callback_data(void* data) noexcept {
        rpc_callback_data* cb_data = (rpc_callback_data*)data;
        delete cb_data;
    }

    static void on_engine_finalize_cb(void* arg) noexcept {
        auto e               = static_cast<detail::engine_impl*>(arg);
        e->m_mid.store(MARGO_INSTANCE_NULL);
    }

    static void on_engine_prefinalize_cb(void* arg) noexcept {
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
     * @param args pointer to a margo_init_info structure to pass to margo.
     */
    engine(const std::string& addr, int mode,
           const margo_init_info* args)
    : m_impl(std::make_shared<detail::engine_impl>()) {
        m_impl->m_mid = margo_init_ext(addr.c_str(), mode, args);
        if(!m_impl->m_mid)
            MARGO_THROW(margo_init_ext, HG_OTHER_ERROR, "Could not initialize Margo");
        m_impl->m_owns_mid = true;
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
    }

    /**
     * @brief Constructor.
     *
     * @param addr address of this instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     * @param use_progress_thread whether to use a dedicated ES to drive
     * progress.
     * @param rpc_thread_count number of threads to use for servicing RPCs.
     * Use -1 to indicate that RPCs should be serviced in the progress ES.
     * @param hg_opt options for initializing Mercury.
     */
    engine(const std::string& addr, int mode, bool use_progress_thread = false,
           std::int32_t rpc_thread_count = 0, const hg_init_info *hg_opt = nullptr)
    : m_impl(std::make_shared<detail::engine_impl>()) {

        std::string config = "{ \"use_progress_thread\" : ";
        config += use_progress_thread ? "true" : "false";
        config += ", \"rpc_thread_count\" : ";
        config += std::to_string(rpc_thread_count);
        config +=  "}";

        margo_init_info args;
        memset(&args, 0, sizeof(args));
        args.json_config  = config.c_str();
        args.hg_init_info = (hg_init_info*)hg_opt;

        m_impl->m_mid = margo_init_ext(addr.c_str(), mode, &args);
        if(!m_impl->m_mid)
            MARGO_THROW(margo_init_ext, HG_OTHER_ERROR, "Could not initialize Margo");
        m_impl->m_owns_mid = true;
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
    }

    /**
     * @brief Constructor.
     *
     * @param addr address of this instance.
     * @param mode THALLIUM_SERVER_MODE or THALLIUM_CLIENT_MODE.
     * @param config JSON configuration.
     * @param hg_opt options for initializing Mercury.
     * Use -1 to indicate that RPCs should be serviced in the progress ES.
     */
    engine(const std::string& addr, int mode,
           const char* config, const hg_init_info *hg_opt = nullptr)
    : m_impl(std::make_shared<detail::engine_impl>()) {

        margo_init_info args;
        memset(&args, 0, sizeof(args));
        args.json_config  = config;
        args.hg_init_info = (hg_init_info*)hg_opt;

        m_impl->m_mid = margo_init_ext(addr.c_str(), mode, &args);
        if(!m_impl->m_mid)
            MARGO_THROW(margo_init_ext, HG_OTHER_ERROR, "Could not initialize Margo");
        m_impl->m_owns_mid = true;
        margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
                                        static_cast<void*>(m_impl.get()));
        margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
                                     static_cast<void*>(m_impl.get()));
    }

    engine(const std::string& addr, int mode,
           const std::string& config, const hg_init_info *hg_opt = nullptr)
    : engine(addr, mode, config.c_str(), hg_opt) {}

    engine(const std::string& addr, int mode, const pool& progress_pool,
           const pool& default_handler_pool);

    /**
     * @brief Builds an engine around an existing margo instance.
     *
     * @param mid Margo instance.
     */
    engine(margo_instance_id mid) noexcept
    : m_impl(std::make_shared<detail::engine_impl>()) {
        m_impl->m_mid       = mid;
        m_impl->m_owns_mid  = false;
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
        margo_instance_id mid = m_impl->m_mid;
        if(mid == MARGO_INSTANCE_NULL) return;
        if(m_impl->m_owns_mid) {
            if(margo_is_listening(mid)) {
                wait_for_finalize();
            } else {
                finalize();
            }
        }
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
        margo_instance_id mid = m_impl->m_mid;
        if(mid != MARGO_INSTANCE_NULL)
            margo_wait_for_finalize(mid);
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
    remote_procedure define(const char* name);

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
           uint16_t provider_id, const pool& p);

    template <typename A1, typename... Args>
    remote_procedure
    define(const std::string&                                      name,
           const std::function<void(const request&, A1, Args...)>& fun,
           uint16_t provider_id = 0);

    template <typename A1, typename... Args>
    remote_procedure
    define(const std::string&                                 name,
           std::function<void(const request&, A1, Args...)>&& fun,
           uint16_t provider_id, const pool& p);

    template <typename A1, typename... Args>
    remote_procedure
    define(const std::string&                                 name,
           std::function<void(const request&, A1, Args...)>&& fun,
           uint16_t provider_id = 0);

    template <typename Func, typename ... Extra>
    typename std::enable_if<
        !is_std_function_object<typename std::decay<Func>::type>::value
        && !std::is_function<typename std::remove_pointer<typename std::decay<Func>::type>::type>::value, remote_procedure>::type
    define(const std::string& name, Func&& fun, const Extra&... extra) {
        using function = typename std::function<typename function_signature<Func>::type>;
        return define(name, function(std::forward<Func>(fun)), extra...);
    }

    remote_procedure define(const std::string&                         name,
                            const std::function<void(const request&)>& fun,
                            uint16_t provider_id, const pool& p);

    remote_procedure define(const std::string&                         name,
                            const std::function<void(const request&)>& fun,
                            uint16_t provider_id = 0);

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
                            uint16_t provider_id, const pool& p);

    template <typename... Args>
    remote_procedure define(const std::string& name,
                            void (*f)(const request&, Args...),
                            uint16_t provider_id = 0);

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
    void enable_remote_shutdown() {
        if(!m_impl) throw exception("Invalid engine");
         margo_enable_remote_shutdown(m_impl->m_mid);
    }

    /**
     * @brief Checks if the engine is listening for incoming RPCs.
     */
    bool is_listening() const {
        if(!m_impl) {
            throw exception("Invalid engine");
        }
        return margo_is_listening(m_impl->m_mid);
    }

    /**
     * @brief Get the handler's pool.
     *
     * @return The default pool used for RPC handlers.
     */
    pool get_handler_pool() const;

    /**
     * @brief Get the progress pool.
     *
     * @return The pool used for network progress.
     */
    pool get_progress_pool() const;

    /**
     * @brief Create a timed_callback object linked to the engine.
     *
     * @tparam F Callback type.
     * @param cb Callback
     *
     * @return a timed_callback object.
     */
    template<typename F>
    timed_callback create_timed_callback(F&& cb) const;

    /**
     * @brief Get the JSON configuration of the internal
     * Margo instance.
     */
    std::string get_config() const {
        if(!m_impl) throw exception("Invalid engine");
        char* cfg = margo_get_config(m_impl->m_mid);
        if(cfg) {
            auto result = std::string(cfg);
            free(cfg);
            return result;
        }
        else return std::string();
    }

    template<typename T>
    struct named_object_proxy : public T {

        named_object_proxy(T&& obj, std::string name, unsigned index)
        : T(std::move(obj))
        , m_name(std::move(name))
        , m_index(index) {}

        named_object_proxy(const named_object_proxy&) = default;
        named_object_proxy(named_object_proxy&&) = default;
        named_object_proxy& operator=(const named_object_proxy&) = default;
        named_object_proxy& operator=(named_object_proxy&&) = default;
        ~named_object_proxy() = default;

        const auto& name() const {
            return m_name;
        }

        const auto& index() const {
            return m_index;
        }

        private:

        std::string m_name;
        unsigned m_index;
    };

    /**
     * @private
     * @brief The list_proxy template class is used to return
     * a proxy list to the internal list of either xstreams or pools.
     *
     * @tparam T thallium::xstream or thallium::pool
     * @tparam Native ABT_xstream or ABT_pool
     */
    template<typename T, typename Native>
    struct list_proxy {

        ~list_proxy() = default;

        /**
         * @brief Lookup the object by its name.
         * Throws std::out_of_range if the name is unknown.
         */
        auto operator[](const char* name) const {
            Native handle;
            int ret = m_get_by_name(m_mid, name, &handle);
            if(ret != 0) throw std::out_of_range("unknown object name in list_proxy");
            int index = m_get_index(m_mid, name);
            return named_object_proxy<T>(T(handle), std::string(name), index);
        }

        /**
         * @brief Lookup the object by its name.
         * Throws std::out_of_range if the name is unknown.
         */
        auto operator[](const std::string& name) const {
            return (*this)[name.c_str()];
        }

        /**
         * @brief Lookup the object by its index.
         * Throws std::out_of_range if the name is unknown.
         */
        template<typename I,
                 std::enable_if_t<std::is_integral<I>::value, bool> = true>
        auto operator[](I index) const {
            Native handle;
            int ret = m_get_by_index(m_mid, index, &handle);
            if(ret != 0) throw std::out_of_range("index out of range in list_proxy");
            const char* name = m_get_name(m_mid, index);
            return named_object_proxy<T>(T(handle), std::string(name), index);
        }

        /**
         * @brief Return the number of elements this proxy can access.
         */
        std::size_t size() const {
            return m_get_num(m_mid);
        }

        private:

        friend class engine;

        typedef int (*get_by_name_f)(margo_instance_id, const char*, Native*);
        typedef int (*get_by_index_f)(margo_instance_id, unsigned, Native*);
        typedef const char* (*get_name_f)(margo_instance_id, unsigned);
        typedef int (*get_index_f)(margo_instance_id, const char*);
        typedef size_t (*get_num_f)(margo_instance_id);

        margo_instance_id m_mid;
        get_by_name_f     m_get_by_name;
        get_by_index_f    m_get_by_index;
        get_name_f        m_get_name;
        get_index_f       m_get_index;
        get_num_f         m_get_num;

        list_proxy(margo_instance_id mid,
                   get_by_name_f get_by_name,
                   get_by_index_f get_by_index,
                   get_name_f get_name,
                   get_index_f get_index,
                   get_num_f get_num)
        : m_mid(mid)
        , m_get_by_name(get_by_name)
        , m_get_by_index(get_by_index)
        , m_get_name(get_name)
        , m_get_index(get_index)
        , m_get_num(get_num) {}

        list_proxy(const list_proxy&) = default;
        list_proxy(list_proxy&&) = default;
        list_proxy& operator=(const list_proxy&) = default;
        list_proxy& operator=(list_proxy&&) = default;
    };

    /**
     * @brief Returns a proxy object that can be used to access
     * the internal list of xstreams in the underlying margo instance.
     */
    auto xstreams() const {
        if(!m_impl) {
            throw exception("Invalid engine");
        }
        return list_proxy<xstream, ABT_xstream>{
            m_impl->m_mid,
            &margo_get_xstream_by_name,
            &margo_get_xstream_by_index,
            &margo_get_xstream_name,
            &margo_get_xstream_index,
            &margo_get_num_xstreams};
    }

    /**
     * @brief Returns a proxy object that can be used to access
     * the internal list of pools in the underlying margo instance.
     */
    auto pools() const {
        if(!m_impl) {
            throw exception("Invalid engine");
        }
        return list_proxy<pool, ABT_pool>{
            m_impl->m_mid,
            &margo_get_pool_by_name,
            &margo_get_pool_by_index,
            &margo_get_pool_name,
            &margo_get_pool_index,
            &margo_get_num_pools};
    }

    void set_logger(logger* l) {
        if(!m_impl) {
            throw exception("Invalid engine");
        }
        margo_logger ml = {
            static_cast<void*>(l),
            &logger::log_trace,
            &logger::log_debug,
            &logger::log_info,
            &logger::log_warning,
            &logger::log_error,
            &logger::log_critical
        };
        if(0 != margo_set_logger(m_impl->m_mid, &ml)) {
            throw exception("Cannot set engine logger");
        }
    }

    void set_log_level(logger::level l) {
        if(!m_impl) {
            throw exception("Invalid engine");
        }
        if(0 != margo_set_log_level(m_impl->m_mid, static_cast<margo_log_level>(l))) {
            throw exception("Cannot set engine log level");
        }
    }
};

} // namespace thallium

#include <thallium/bulk.hpp>
#include <thallium/request.hpp>
#include <thallium/proc_object.hpp>
#include <thallium/pool.hpp>
#include <thallium/remote_procedure.hpp>
#include <thallium/timed_callback.hpp>
#include <thallium/serialization/proc_input_archive.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/stl/tuple.hpp>

namespace thallium {

inline engine::engine(const std::string& addr, int mode, const pool& progress_pool,
                      const pool& default_handler_pool)
:  m_impl(std::make_shared<detail::engine_impl>()) {
    m_impl->m_owns_mid        = true;

    margo_init_info args;
    memset(&args, 0, sizeof(args));
    args.progress_pool  = progress_pool.native_handle();
    args.rpc_pool       = default_handler_pool.native_handle();

    m_impl->m_mid = margo_init_ext(NULL, mode, &args);

    if(!m_impl->m_mid)
        MARGO_THROW(margo_init_ext, HG_OTHER_ERROR, "Could not initialize Margo");
    margo_push_prefinalize_callback(m_impl->m_mid, &engine::on_engine_prefinalize_cb,
            static_cast<void*>(m_impl.get()));
    margo_push_finalize_callback(m_impl->m_mid, &engine::on_engine_finalize_cb,
            static_cast<void*>(m_impl.get()));
}

template <typename T1, typename... Tn>
remote_procedure
engine::define(const std::string&                               name,
               std::function<void(const request&, T1, Tn...)>&& fun,
               uint16_t provider_id, const pool& p) {
    if(!m_impl) throw exception("Invalid engine");
    hg_id_t id = MARGO_REGISTER_PROVIDER(
        m_impl->m_mid, name.c_str(), meta_serialization, meta_serialization,
        thallium_generic_rpc, provider_id, p.native_handle());

    std::weak_ptr<detail::engine_impl> w_impl = m_impl;
    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine_impl = m_impl;

    cb_data->m_function =
        [fun=std::move(fun), w_impl=std::move(w_impl)](const request& r) {
            std::function<void(T1, Tn...)> call_function =
                [&fun, &r](const T1& a1, const Tn&... args) {
                    fun(r, a1, args...);
                };
                std::tuple<typename std::decay<T1>::type,
                   typename std::decay<Tn>::type...> iargs;
            meta_proc_fn mproc = [w_impl, &iargs](hg_proc_t proc) {
                auto ctx = std::tuple<>(); // TODO make this context available as argument
                return proc_object_decode(proc, iargs, w_impl, ctx);
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

    hg_return_t ret =
        margo_register_data(m_impl->m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(m_impl, id);
}

template <typename T1, typename... Tn>
remote_procedure
engine::define(const std::string&                               name,
               std::function<void(const request&, T1, Tn...)>&& fun,
               uint16_t provider_id) {
    return define(name, std::move(fun), provider_id, pool());
}

template <typename T1, typename... Tn>
remote_procedure
engine::define(const std::string&                             name,
               const std::function<void(const request&, T1, Tn...)>& fun,
               uint16_t provider_id, const pool& p) {
    return define(name, std::function<void(const request&, T1, Tn...)>(fun),
                  provider_id, p);
}

template <typename T1, typename... Tn>
remote_procedure
engine::define(const std::string&                             name,
               const std::function<void(const request&, T1, Tn...)>& fun,
               uint16_t provider_id) {
    return define(name, std::function<void(const request&, T1, Tn...)>(fun),
                  provider_id, pool());
}

template <typename... Args>
remote_procedure engine::define(const std::string& name,
                                void (*f)(const request&, Args...),
                                uint16_t provider_id, const pool& p) {
    return define(name, std::function<void(const request&, Args...)>(f),
                  provider_id, p);
}

template <typename... Args>
remote_procedure engine::define(const std::string& name,
                                void (*f)(const request&, Args...),
                                uint16_t provider_id) {
    return define(name, std::function<void(const request&, Args...)>(f),
                  provider_id, pool());
}

inline remote_procedure engine::define(const std::string& name) {
    return define(name.c_str());
}

inline remote_procedure engine::define(const char* name) {
    hg_bool_t flag;
    hg_id_t   id;
    if(!m_impl) throw exception("Invalid engine");
    margo_registered_name(m_impl->m_mid, name, &id, &flag);
    if(flag == HG_FALSE) {
        id = MARGO_REGISTER(m_impl->m_mid, name, meta_serialization, meta_serialization, NULL);
    }
    return remote_procedure(m_impl, id);
}

inline remote_procedure engine::define(const std::string&                         name,
                                       const std::function<void(const request&)>& fun,
                                       uint16_t provider_id, const pool& p) {
    if(!m_impl) throw exception("Invalid engine");
    hg_id_t id = MARGO_REGISTER_PROVIDER(
        m_impl->m_mid, name.c_str(), meta_serialization, meta_serialization,
        thallium_generic_rpc, provider_id, p.native_handle());

    auto* cb_data          = new rpc_callback_data;
    cb_data->m_engine_impl = m_impl;
    cb_data->m_function    = fun;

    hg_return_t ret =
        margo_register_data(m_impl->m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(m_impl, id);
}

inline remote_procedure engine::define(const std::string&                         name,
                                       const std::function<void(const request&)>& fun,
                                       uint16_t provider_id) {
    return define(name, fun, provider_id, pool());
}

inline endpoint engine::lookup(const std::string& address) const {
    if(!m_impl) throw exception("Invalid engine");
    hg_addr_t   addr;
    hg_return_t ret = margo_addr_lookup(m_impl->m_mid, address.c_str(), &addr);
    MARGO_ASSERT(ret, margo_addr_lookup);
    return endpoint(const_cast<engine&>(*this), addr);
}

inline endpoint engine::self() const {
    if(!m_impl) throw exception("Invalid engine");
    hg_addr_t   self_addr;
    hg_return_t ret = margo_addr_self(m_impl->m_mid, &self_addr);
    MARGO_ASSERT(ret, margo_addr_self);
    return endpoint(const_cast<engine&>(*this), self_addr);
}

inline bulk engine::expose(const std::vector<std::pair<void*, size_t>>& segments,
                    bulk_mode                                    flag) {
    if(!m_impl) throw exception("Invalid engine");
    hg_bulk_t              handle;
    hg_uint32_t            count = segments.size();
    std::vector<void*>     buf_ptrs(count);
    std::vector<hg_size_t> buf_sizes(count);
    for(unsigned i = 0; i < segments.size(); i++) {
        buf_ptrs[i]  = segments[i].first;
        buf_sizes[i] = segments[i].second;
    }
    hg_return_t ret = margo_bulk_create(
        m_impl->m_mid, count, &buf_ptrs[0], &buf_sizes[0],
        static_cast<hg_uint32_t>(flag), &handle);
    MARGO_ASSERT(ret, margo_bulk_create);
    return bulk(m_impl, handle, true);
}

inline bulk engine::wrap(hg_bulk_t blk, bool is_local) {
    if(!m_impl) throw exception("Invalid engine");
    hg_return_t hret = margo_bulk_ref_incr(blk);
    MARGO_ASSERT(hret, margo_bulk_ref_incr);
    return bulk(m_impl, blk, is_local);
}

inline void engine::shutdown_remote_engine(const endpoint& ep) const {
    if(!m_impl) throw exception("Invalid engine");
    int         ret = margo_shutdown_remote_instance(m_impl->m_mid, ep.m_addr);
    hg_return_t r   = ret == 0 ? HG_SUCCESS : HG_OTHER_ERROR;
    MARGO_ASSERT(r, margo_shutdown_remote_instance);
}

inline pool engine::get_handler_pool() const {
    if(!m_impl) {
        throw exception("Invalid engine");
    }
    ABT_pool p = ABT_POOL_NULL;
    margo_get_handler_pool(m_impl->m_mid, &p);
    return pool(p);
}

inline pool engine::get_progress_pool() const {
    if(!m_impl) {
        throw exception("Invalid engine");
    }
    ABT_pool p = ABT_POOL_NULL;
    margo_get_progress_pool(m_impl->m_mid, &p);
    return pool(p);
}

template<typename F>
inline timed_callback engine::create_timed_callback(F&& cb) const {
    if(!m_impl) {
        throw exception("Invalid engine");
    }
    return timed_callback(*this, std::forward<F>(cb));
}

inline hg_return_t thallium_generic_rpc(hg_handle_t handle) {
    margo_instance_id mid = margo_hg_handle_get_instance(handle);
    THALLIUM_ASSERT_CONDITION(mid != 0,
            "margo_hg_handle_get_instance returned null");
    const struct hg_info* info = margo_get_info(handle);
    THALLIUM_ASSERT_CONDITION(info != nullptr,
            "margo_get_info returned null");
    void* data = margo_registered_data(mid, info->id);
    THALLIUM_ASSERT_CONDITION(data != nullptr,
            "margo_registered_data returned null");
    auto    cb_data = static_cast<engine::rpc_callback_data*>(data);
    auto&   rpc = cb_data->m_function;
    if(!(cb_data->m_engine_impl.lock())) {
        margo_destroy(handle);
        return HG_OTHER_ERROR;
    }
    request req(cb_data->m_engine_impl, handle, false);
    rpc(req);
    margo_destroy(handle);
    return HG_SUCCESS;
}

inline __MARGO_INTERNAL_RPC_WRAPPER(thallium_generic_rpc)
inline __MARGO_INTERNAL_RPC_HANDLER(thallium_generic_rpc)

} // namespace thallium

#endif
