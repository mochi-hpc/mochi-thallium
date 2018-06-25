/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_THREAD_HPP
#define __THALLIUM_THREAD_HPP

#include <cstdint>
#include <abt.h>
#include <thallium/managed.hpp>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>

namespace thallium {

/**
 * Exception class thrown by the thread class.
 */
class thread_exception : public exception {

    public:

    template<typename ... Args>
    thread_exception(Args&&... args)
    : exception(std::forward<Args>(args)...) {}
};

#define TL_THREAD_EXCEPTION(__fun,__ret) \
        thread_exception(#__fun," returned ", abt_error_get_name(__ret),\
                            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_THREAD_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_THREAD_EXCEPTION(__call, __ret);\
    }\
}

class pool;
class xstream;
class scheduler;

enum class thread_state : std::int32_t {
    ready      = ABT_THREAD_STATE_READY,
    running    = ABT_THREAD_STATE_RUNNING,
    blocked    = ABT_THREAD_STATE_BLOCKED,
    terminated = ABT_THREAD_STATE_TERMINATED
};

/**
 * @brief The thread class wraps an Argobot's ABT_thread resource.
 * Note that the task object does NOT manage the internal ABT_thread.
 * It doesn't do reference counting nor deletes the ABT_thread when
 * the object is destroyed. It simply provides an object-oriented
 * interface to it. To make a thread object that manages its ABT_thread,
 * use managed<thread> instead.
 */
class thread {

    friend class pool;
    friend class xstream;
    friend class managed<thread>;

    public:

    class attribute {

        friend class thread;

        ABT_thread_attr m_attr;

        /**
         * @brief Makes an attribute object from an ABT_thread_attr.
         * Used by thread::get_attribute().
         *
         * @param attr ABT_thread_attr handle.
         */
        explicit attribute(ABT_thread_attr attr)
            : m_attr(attr) {}

        public:

        /**
         * @brief Native handle type.
         */
        typedef ABT_thread_attr native_handle_type;

        /**
         * @brief Constructor.
         */
        attribute() {
            TL_THREAD_ASSERT(ABT_thread_attr_create(&m_attr));
        }

        /**
         * @brief Copy-constructor is deleted.
         */
        attribute(const attribute&) = delete;

        /**
         * @brief Move constructor.
         */
        attribute(attribute&& other) 
        : m_attr(other.m_attr) {
            other.m_attr = ABT_THREAD_ATTR_NULL;
        }

        /**
         * @brief Copy-assignment operator is deleted.
         */
        attribute& operator=(const attribute&) = delete;

        /**
         * @brief Move-assignment operator.
         */
        attribute& operator=(attribute&& other) {
            if(this == &other)
                return *this;
            if(m_attr != ABT_THREAD_ATTR_NULL) {
                TL_THREAD_ASSERT(ABT_thread_attr_free(&m_attr));
            }
            m_attr = other.m_attr;
            other.m_attr = ABT_THREAD_ATTR_NULL;
            return *this;
        }

        /**
         * @brief Destructor.
         */
        ~attribute() {
            if(m_attr != ABT_THREAD_ATTR_NULL) {
                ABT_thread_attr_free(&m_attr);
            }
        }

        /**
         * @brief Get the underlying native handle.
         *
         * @return the native handle.
         */
        native_handle_type native_handle() const {
            return m_attr;
        }

        /**
         * @brief Sets the address and size of the stack to use.
         *
         * @param addr address of the stack.
         * @param size size of the stack.
         */
        void set_stack(void* addr, size_t size) {
            TL_THREAD_ASSERT(ABT_thread_attr_set_stack(m_attr, addr, size));
        }

        /**
         * @brief Gets the address of the stack.
         *
         * @return the address of the stack.
         */
        void* get_stack_address() const {
            void* addr;
            size_t size;
            TL_THREAD_ASSERT(ABT_thread_attr_get_stack(m_attr, &addr, &size));
            return addr;
        }

        /**
         * @brief Get the size of the stack.
         *
         * @return the size of the stack.
         */
        size_t get_stack_size() const {
            void* addr;
            size_t size;
            TL_THREAD_ASSERT(ABT_thread_attr_get_stack(m_attr, &addr, &size));
            return size;
        }

        /**
         * @brief Set the migratability of the stack.
         */
        void set_migratable(bool migratable) {
            ABT_bool flag = migratable ? ABT_TRUE : ABT_FALSE;
            TL_THREAD_ASSERT(ABT_thread_attr_set_migratable(m_attr, flag));
        }

    };

    private:

	ABT_thread m_thread;

    thread(ABT_thread t)
    : m_thread(t) {}

    static managed<thread> create_on_xstream(ABT_xstream es, void(*f)(void*), void* arg) {
        ABT_thread t;
        TL_THREAD_ASSERT(ABT_thread_create_on_xstream(es, f, arg, ABT_THREAD_ATTR_NULL, &t));
        return managed<thread>(t);
    }

    static managed<thread> create_on_pool(ABT_pool p, void(*f)(void*), void* arg) {
        ABT_thread t;
        TL_THREAD_ASSERT(ABT_thread_create(p, f, arg, ABT_THREAD_ATTR_NULL, &t));
        return managed<thread>(t);
    }

    static managed<thread> create_on_xstream(ABT_xstream es, void(*f)(void*), void* arg, const attribute& attr) {
        ABT_thread t;
        TL_THREAD_ASSERT(ABT_thread_create_on_xstream(es, f, arg, attr.native_handle(), &t));
        return managed<thread>(t);
    }

    static managed<thread> create_on_pool(ABT_pool p, void(*f)(void*), void* arg, const attribute& attr) {
        ABT_thread t;
        TL_THREAD_ASSERT(ABT_thread_create(p, f, arg, attr.native_handle(), &t));
        return managed<thread>(t);
    }
    void destroy() {
        if(m_thread != ABT_THREAD_NULL) 
            ABT_thread_free(&m_thread);
	}

	public:

    /**
     * @brief Native handle type.
     */
    typedef ABT_thread native_handle_type;

    /**
     * @brief Default constructor.
     */
    thread()
    : m_thread(ABT_THREAD_NULL) {}

    /**
     * @brief Copy constructor.
     */
	thread(const thread& other) = default;

    /**
     * @brief Copy-assignment operator.
     */
    thread& operator=(const thread& other) = default;

    /**
     * @brief Move-assignment operator. The object
     * moved from will be invalidated if different from
     * the object moved to.
     */
    thread& operator=(thread&& other) {
        if(this == &other) return *this;
        m_thread = other.m_thread;
        other.m_thread = ABT_THREAD_NULL;
        return *this;
    }

    /**
     * @brief Move constructor. The object moved from will
     * be invalidated.
     */
	thread(thread&& other) {
		m_thread = other.m_thread;
		other.m_thread = ABT_THREAD_NULL;
	}

    /**
     * @brief Destructor.
     */
	~thread() = default;

    /**
     * @brief Blocks until thread terminates.
     * Since this routine blocks, only ULTs can call this routine.
     * If tasks use this routine, the behavior is undefined.
     */
    void join() {
        TL_THREAD_ASSERT(ABT_thread_join(m_thread));
    }

    /**
     * @brief Request the cancelation of the thread.
     */
    void cancel() {
        TL_THREAD_ASSERT(ABT_thread_cancel(m_thread));
    }

    /**
     * @brief Returns the native handle of this thread.
     *
     * @return the native handle of this thread.
     */
    ABT_thread native_handle() const {
        return m_thread;
    }

    /**
     * @brief Get the id of this thread.
     *
     * @return the id this thread.
     */
    std::uint64_t id() const {
        std::uint64_t id;
        TL_THREAD_ASSERT(ABT_thread_get_id(m_thread, &id));
        return id;
    }

    /**
     * @brief Get the state of this thread.
     *
     * @return the state of this thread.
     */
    thread_state state() const {
        ABT_thread_state stt;
        TL_THREAD_ASSERT(ABT_thread_get_state(m_thread, &stt));
        return (thread_state)stt;
    }

    /**
     * @brief Get the stack size of this thread.
     *
     * @return the stack size of this thread.
     */
    std::size_t stacksize() const {
        std::size_t s;
        TL_THREAD_ASSERT(ABT_thread_get_stacksize(m_thread, &s));
        return s;
    }

    attribute get_attribute() const {
        ABT_thread_attr attr;
        TL_THREAD_ASSERT(ABT_thread_get_attr(m_thread, &attr));
        return attribute(attr);
    }

    /**
     * @brief Sets the thread's migratability. By default, all threads 
     * are migratable. If flag is true, the thread becomes migratable.
     * On the other hand, if flag is false, the thread
     * becomes unmigratable.
     *
     * @param flag true to make the thread migratable, false otherwise.
     */
    void set_migratable(bool flag) {
        ABT_bool b = flag ? ABT_TRUE : ABT_FALSE;
        TL_THREAD_ASSERT(ABT_thread_set_migratable(m_thread, b));
    }

    /**
     * @brief Returns whether the thread is migratable or not.
     *
     * @return true if the task can be migrated, false otherwise.
     */
    bool is_migratable() const {
        ABT_bool flag;
        TL_THREAD_ASSERT(ABT_thread_is_migratable(m_thread, &flag));
    }

    /**
     * @brief confirms whether the thread, is the primary ULT.
     *
     * @return true if the thread is the primary ULT, false otherwise. 
     */
    bool is_primary() const {
        ABT_bool flag;
        TL_THREAD_ASSERT(ABT_thread_is_primary(m_thread, &flag));
    }

    /**
     * @brief Comparison operator for threads.
     *
     * @param other Thread to compare against.
     *
     * @return true if the two threads are equal, false otherwise.
     */
    bool operator==(const thread& other) const {
        ABT_bool b;
        TL_THREAD_ASSERT(ABT_thread_equal(m_thread, other.m_thread, &b));
        return b == ABT_TRUE;
    }

    /**
     * @brief Makes the blocked ULT schedulable by changing the
     * state of the target ULT to READY and pushing it to its 
     * associated pool. The ULT will resume its execution when
     * the scheduler schedules it.
     */
    void resume() {
        TL_THREAD_ASSERT(ABT_thread_resume(m_thread));
    }

    /**
     * @brief Requests migration of the thread but does not 
     * specify the target ES. The target ES will be determined 
     * among available ESs by the runtime.
     */
    void migrate() {
        TL_THREAD_ASSERT(ABT_thread_migrate(m_thread));
    }

    /**
     * @brief Requests migration of the thread to the provided
     * ES. The actual migration occurs asynchronously with this
     * function call. In other words, this function may return
     * immediately without the thread being migrated. The migration
     * request will be posted on the thread, such that next time a
     * scheduler picks it up, migration will happen. The target
     * pool is chosen by the running scheduler of the target ES.
     * The migration will fail if the running scheduler has no pool
     * available for migration.
     *
     * @param es ES to migrate to.
     */
    void migrate_to(xstream& es);
    
    /**
     * @brief Migrate a thread to a specific scheduler.
     *
     * The actual migration occurs asynchronously with this function call.
     * In other words, this function may return immediately without the
     * thread being migrated. The migration request will be posted on the
     * thread, such that next time a scheduler picks it up, migration will
     * happen. The target pool is chosen by the scheduler itself. The
     * migration will fail if the target scheduler has no pool available
     * for migration.
     *
     * @param sched scheduler to migrate the thread to.
     */
    void migrate_to(scheduler& sched);

    /**
     * @brief Migrate a thread to a specific pool.
     * 
     * The actual migration occurs asynchronously with this function call.
     * In other words, this function may return immediately without the
     * thread being migrated. The migration request will be posted on the
     * thread, such that next time a scheduler picks it up, migration
     * will happen.
     *
     * @param p pool to migrate the thread to.
     */
    void migrate_to(pool& p);

    /**
     * @brief If the thread is not running, returns the pool where it is,
     * else returns the last pool where it was (the pool from which the 
     * thread was popped).
     *
     * @return the last pool that had the thread.
     */
    pool get_last_pool() const;

    /**
     * @brief Get the id of the last pool this thread belonged to.
     *
     * @return the id of the last pool that had the thread.
     */
    int get_last_pool_id() const {
        int id;
        TL_THREAD_ASSERT(ABT_thread_get_last_pool_id(m_thread, &id));
        return id;
    }

    /**
     * @brief Returns the handle of the calling thread.
     * If tasks call this routine, a null thread will be returned.
     *
     * @return the handle to the calling thread.
     */
    static thread self() {
        ABT_thread t;
        TL_THREAD_ASSERT(ABT_thread_self(&t));
        return thread(t);
    }

    /**
     * @brief Returns the id of the calling thread.
     * If tasks call this routine, the result is undefined.
     *
     * @return the id of the calling thread.
     */
    static std::uint64_t self_id() {
        std::uint64_t id;
        TL_THREAD_ASSERT(ABT_thread_self_id(&id));
        return id;
    }

    /**
     * @brief The calling ULT terminates its execution.
     * Since the calling ULT terminates, this routine never returns.
     */
    static void exit() {
        TL_THREAD_ASSERT(ABT_thread_exit());
    }

    /**
     * @brief Yield the processor from the current running ULT back 
     * to the scheduler. The ULT that yields, goes back to its pool,
     * and eventually will be resumed automatically later.
     */
    static void yield() {
        TL_THREAD_ASSERT(ABT_thread_yield());
    }

    /**
     * Yield the processor from the current running thread
     * to the specific thread. This function can be used for users
     * to explicitly schedule the next thread to execute.
     *
     * @param other thread to yield to.
     */
    static void yield_to(const thread& other) {
        TL_THREAD_ASSERT(ABT_thread_yield_to(other.m_thread));
    }
};

}

#undef TL_THREAD_EXCEPTION
#undef TL_THREAD_ASSERT

#endif /* end of include guard */
