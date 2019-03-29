/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_POOL_HPP
#define __THALLIUM_POOL_HPP

#include <memory>
#include <functional>
#include <abt.h>
#include <thallium/anonymous.hpp>
#include <thallium/task.hpp>
#include <thallium/thread.hpp>
#include <thallium/managed.hpp>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>
#include <thallium/unit_type.hpp>

namespace thallium {

class xstream;
class scheduler;

/**
 * Exception class thrown by the pool class.
 */
class pool_exception : public exception {

    public:

    template<typename ... Args>
    pool_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_POOL_EXCEPTION(__fun,__ret) \
    pool_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_POOL_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_POOL_EXCEPTION(__call, __ret);\
    }\
}

/**
 * @brief Wrapper for Argobots' ABT_pool.
 * 
 * NOTE: This class does not provide all the functionalities of
 * Argobot's pool, in particular custom definitions.
 */
class pool {

    public:

    /**
     * @brief Types of accesses enabled by the pool:
     * private, single-producer-single-consumer,
     * multiple-producer-single-consumer, single-
     * producer-multiple-consumer, or multiple-producer-
     * multiple-consumer.
     */
    enum class access : std::int32_t {
        priv = ABT_POOL_ACCESS_PRIV, 
        spsc = ABT_POOL_ACCESS_SPSC,
        mpsc = ABT_POOL_ACCESS_MPSC,
        spmc = ABT_POOL_ACCESS_SPMC,
        mpmc = ABT_POOL_ACCESS_MPMC
    };

    private:

    template<typename P, typename U,
        typename Palloc = std::allocator<P>,
        typename Ualloc = std::allocator<U>>
    struct pool_def {

        private:
                                
            static Palloc pool_allocator;
            static Ualloc unit_allocator;

        public:

            static ABT_unit_type u_get_type(ABT_unit u) {
                auto uu = static_cast<U*>(u);
                return (ABT_unit_type)(uu->get_type());
            }

            static ABT_thread u_get_thread(ABT_unit u) {
                auto uu = static_cast<U*>(u);
                return uu->get_thread().native_handle();
            }

            static ABT_task u_get_task(ABT_unit u) {
                auto uu = static_cast<U*>(u);
                return uu->get_task().native_handle();
            }

            static ABT_bool u_is_in_pool(ABT_unit u) {
                auto uu = static_cast<U*>(u);
                return (ABT_bool)(uu->is_in_pool());
            }

            static ABT_unit u_create_from_thread(ABT_thread t) {
                auto uu = std::allocator_traits<Ualloc>::allocate(unit_allocator,1);
                std::allocator_traits<Ualloc>::construct(unit_allocator, uu, thread(t));
                return  static_cast<ABT_unit>(uu);
            }

            static ABT_unit u_create_from_task(ABT_task t) {
                auto uu = std::allocator_traits<Ualloc>::allocate(unit_allocator,1);
                std::allocator_traits<Ualloc>::construct(unit_allocator, uu, task(t));
                return  static_cast<ABT_unit>(uu);
            }

            static void u_free(ABT_unit* u) {
                auto uu = static_cast<U*>(*u);
                std::allocator_traits<Ualloc>::destroy(unit_allocator, uu);
                std::allocator_traits<Ualloc>::deallocate(unit_allocator, uu, 1);
                *u = nullptr;
            }

            static int p_init(ABT_pool p, ABT_pool_config cfg) {
                P* impl = std::allocator_traits<Palloc>::allocate(pool_allocator, 1);
                std::allocator_traits<Palloc>::construct(pool_allocator, impl);
                int ret = ABT_pool_set_data(p, static_cast<void*>(impl));
                return ret;
            }

            static size_t p_get_size(ABT_pool p) {
                void* data;
                int ret = ABT_pool_get_data(p, &data);
                auto impl = static_cast<P*>(data);
                return impl->get_size();
            }

            static void p_push(ABT_pool p, ABT_unit u) {
                void* data;
                int ret = ABT_pool_get_data(p, &data);
                auto impl = static_cast<P*>(data);
                impl->push(static_cast<U*>(u));
            }

            static int p_remove(ABT_pool p, ABT_unit u) {
                void* data;
                int ret = ABT_pool_get_data(p, &data);
                auto impl = static_cast<P*>(data);
                impl->remove(static_cast<U*>(u));
                return ret;
            }

            static ABT_unit p_pop(ABT_pool p) {
                void* data;
                int ret = ABT_pool_get_data(p, &data);
                auto impl = static_cast<P*>(data);
                U* u = impl->pop();
                return static_cast<ABT_unit>(u);
            }

            static int p_free(ABT_pool p) {
                void* data;
                int ret = ABT_pool_get_data(p, &data);
                if(ret != ABT_SUCCESS)
                    return ret;
                auto impl = static_cast<P*>(data);
                std::allocator_traits<Palloc>::destroy(pool_allocator, impl);
                std::allocator_traits<Palloc>::deallocate(pool_allocator, impl, 1);
                return ret;
            }
    };

    ABT_pool m_pool;

    friend class managed<pool>;
    friend class xstream;
    friend class scheduler;
    friend class task;
    friend class thread;

    static void forward_work_unit(void* fp) {
        auto f = static_cast<std::function<void(void)>*>(fp);
        (*f)();
        delete f;
    }

    void destroy() {
        // XXX for now the "automatic" param in ABT pool creation is ignored
        // so if we free things here we will end up with double-free corruptions
//        if(m_pool != ABT_POOL_NULL)
//            ABT_pool_free(&m_pool);
    }

    public:

    /**
     * @brief Constructor used to build a pool  out of an existing handle. 
     *
     * @param p existing ABT_pool handle. May be null.
     */
    explicit pool(ABT_pool p)
    : m_pool(p) {}

    /**
     * @brief Default constructor handles a null pool.
     */
    pool()
    : m_pool(ABT_POOL_NULL) {}

    /**
     * @brief Type of the underlying native handle.
     */
    typedef ABT_pool native_handle_type;

    /**
     * @brief Get the underlying native handle.
     *
     * @return the underlying native handle.
     */
    native_handle_type native_handle() const noexcept {
        return m_pool;
    }


    template<access A, typename P, typename U,
        typename Palloc = std::allocator<P>,
        typename Ualloc = std::allocator<U>>
    static managed<pool> create() {
        using d = pool_def<P,U,Palloc,Ualloc>;
        ABT_pool_def def;
        def.access                  = (ABT_pool_access)A;
        def.u_get_type              = d::u_get_type;
        def.u_get_thread            = d::u_get_thread;
        def.u_get_task              = d::u_get_task;
        def.u_is_in_pool            = d::u_is_in_pool;
        def.u_create_from_thread    = d::u_create_from_thread;
        def.u_create_from_task      = d::u_create_from_task;
        def.u_free                  = d::u_free;
        def.p_init                  = d::p_init;
        def.p_get_size              = d::p_get_size;
        def.p_push                  = d::p_push;
        def.p_pop                   = d::p_pop;
        def.p_remove                = d::p_remove;
        def.p_free                  = d::p_free;
        ABT_pool p;
        TL_POOL_ASSERT(ABT_pool_create(&def, ABT_POOL_CONFIG_NULL, &p));
        return managed<pool>(p);
    }

    /**
     * @brief Constructor.
     *
     * @param access Access type enabled by the pool.
     */
    static managed<pool> create(access a) {
        ABT_pool p;
        TL_POOL_ASSERT(ABT_pool_create_basic(ABT_POOL_FIFO, (ABT_pool_access)a, ABT_FALSE, &p));
        return managed<pool>(p);
    }

    /**
     * @brief Copy constructor.
     */
    pool(const pool& other) = default;

    /**
     * @brief Move constructor.
     */
    pool(pool&& other)
    : m_pool(other.m_pool) {
        other.m_pool = ABT_POOL_NULL;
    }

    /**
     * @brief Copy assignment operator.
     */
    pool& operator=(const pool& other) = default;

    /**
     * @brief Move assignment operator.
     */
    pool& operator=(pool&& other) {
        if(this == &other) return *this;
        m_pool       = other.m_pool;
        other.m_pool = ABT_POOL_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~pool() = default;
    

    /**
     * @brief Check if the underlying pool handle is null.
     *
     * @return true if the pool handle is null, false otherwise.
     */
    bool is_null() const {
        return m_pool == ABT_POOL_NULL;
    }

    /**
     * @brief Returns true if the pool handle is not null.
     */
    operator bool() const {
        return !is_null();
    }

    /**
     * @brief Get the access type of the pool.
     *
     * @return the access type of the pool.
     */
    access get_access() const {
        ABT_pool_access a;
        TL_POOL_ASSERT(ABT_pool_get_access(m_pool, &a));
        return (access)a;
    }

    /**
     * @brief Get the total number of elements
     * present in the pool, including blocked ULTs
     * and migrating ULTs.
     *
     * @return total number of elements in the pool.
     */
    std::size_t total_size() const {
        std::size_t s;
        TL_POOL_ASSERT(ABT_pool_get_total_size(m_pool, &s));
        return s;
    }

    /**
     * @brief Get the number of elements in the pool,
     * not including the ULTs that are blocked.
     *
     * @return the number of elements in the pool.
     */
    std::size_t size() const {
        std::size_t s;
        TL_POOL_ASSERT(ABT_pool_get_size(m_pool, &s));
        return s;
    }

    /**
     * @brief Get the id of the pool.
     *
     * @return the id of the pool.
     */
    int id() const {
        int i;
        TL_POOL_ASSERT(ABT_pool_get_id(m_pool, &i));
        return i;
    }

    /**
     * @brief Pops a unit of work out of the pool.
     *
     * @tparam U Type of the unit of work.
     *
     * @return A pointer to a unit of work.
     */
    template<typename U>
    inline U* pop() {
        ABT_unit u;
        TL_POOL_ASSERT(ABT_pool_pop(m_pool, &u));
        return static_cast<U*>(u);
    }

    /**
     * @brief Pushes a unit of work into the pool.
     * The pool must be a custom pool that manages units of type U.
     * The work unit must have been popped from a pool managing the
     * same type of work units U. The work unit must not have been
     * created manually.
     *
     * @tparam U type of work unit.
     * @param unit work unit.
     */
    template<typename U>
    inline void push(U* unit) {
        TL_POOL_ASSERT(ABT_pool_push(m_pool, static_cast<ABT_unit>(unit)));
    }

    /**
     * @brief Removes a work unit from the pool.
     * The pool must be a custom pool that manages units of type U.
     *
     * @tparam U type of work unit.
     * @param unit work unit.
     */
    template<typename U>
    inline void remove(U* unit) {
        TL_POOL_ASSERT(ABT_pool_remove(m_pool, static_cast<ABT_unit>(unit)));
    }

    /**
     * @brief This function should be called inside a custom scheduler
     * to run a work unit on the ES the scheduler runs on. The type of work
     * unit U should match the type used by the custom pool.
     *
     * @tparam U type of work unit.
     * @param unit Work unit.
     */
    template<typename U>
    inline void run_unit(U* unit) {
        TL_POOL_ASSERT(ABT_xstream_run_unit(static_cast<ABT_unit>(unit), m_pool));
    }

    /**
     * @brief Push a scheduler to a pool.
     * By pushing a scheduler, the user can change the running scheduler:
     * when the top scheduler (the running scheduler) will pick it from 
     * the pool and run it, it will become the new scheduler. This new 
     * scheduler will be in charge until it explicitly yields, except 
     * if scheduler::finish() or scheduler::exit() are called.
     *
     * @param sched Scheduler to push.
     */
    void add_sched(const scheduler& sched);

    /**
     * @brief Create a task running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a task. Must have operator()().
     * @param f Function to run as a task.
     *
     * @return a task object managing the created task.
     */
    template<typename F>
    managed<task> make_task(F&& f) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return task::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp));
    }

    template<typename F>
    void make_task(F&& f, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        task::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp), a);
    }

    /**
     * @brief Create a thread running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a task. Must have operator()().
     * @param f Function to run as a task.
     *
     * @return a thread object managing the created thread.
     */
    template<typename F>
    managed<thread> make_thread(F&& f) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return thread::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp));
    }

    template<typename F>
    void make_thread(F&& f, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        thread::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp), a);
    }

    /**
     * @brief Create a thread running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a task. Must have operator()().
     * @param f Function to run as a task.
     * @param attr Thread attributes.
     *
     * @return a thread object managing the created thread.
     */
    template<typename F>
    managed<thread> make_thread(F&& f, const thread::attribute& attr) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return thread::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp), attr);
    }

    template<typename F>
    void make_thread(F&& f, const thread::attribute& attr, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
         thread::create_on_pool(m_pool, forward_work_unit, static_cast<void*>(fp), attr, a);
    }
};

template<typename P, typename U, typename Palloc, typename Ualloc>
Palloc pool::pool_def<P,U,Palloc,Ualloc>::pool_allocator;

template<typename P, typename U, typename Palloc, typename Ualloc>
Ualloc pool::pool_def<P,U,Palloc,Ualloc>::unit_allocator;

}

#undef TL_POOL_EXCEPTION
#undef TL_POOL_ASSERT

#endif /* end of include guard */
