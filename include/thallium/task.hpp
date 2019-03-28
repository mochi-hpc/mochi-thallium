/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_TASK_HPP
#define __THALLIUM_TASK_HPP

#include <cstdint>
#include <abt.h>
#include <thallium/anonymous.hpp>
#include <thallium/managed.hpp>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>

namespace thallium {

/**
 * Exception class thrown by the task class.
 */
class task_exception : public exception {

    public:

    template<typename ... Args>
        task_exception(Args&&... args)
        : exception(std::forward<Args>(args)...) {}
};

#define TL_TASK_EXCEPTION(__fun,__ret) \
    task_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_TASK_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_TASK_EXCEPTION(__call, __ret);\
    }\
}

class pool;
class xstream;

/**
 * @brief State in which a task can be,
 */
enum class task_state : std::int32_t {
    ready      = ABT_TASK_STATE_READY,
    running    = ABT_TASK_STATE_RUNNING,
    terminated = ABT_TASK_STATE_TERMINATED
};

/**
 * @brief The task class wraps an Argobot's ABT_task resource.
 * Note that the task object does NOT manage the internal ABT_task.
 * It doesn't do reference counting nor deletes the ABT_task when
 * the object is destroyed. It simply provides an object-oriented
 * interface to it. To make a task object that manages its ABT_task,
 * use managed<task> instead.
 */
class task {

    friend class pool;
    friend class xstream;
    friend class managed<task>;

    ABT_task m_task;

    task(ABT_task t)
    : m_task(t) {}

    static managed<task> create_on_xstream(ABT_xstream es, void(*f)(void*), void* arg) {
        ABT_task t;
        TL_TASK_ASSERT(ABT_task_create_on_xstream(es, f, arg, &t));
        return managed<task>(t);
    }

    static void create_on_xstream(ABT_xstream es, void(*f)(void*), void* arg, const anonymous&) {
        TL_TASK_ASSERT(ABT_task_create_on_xstream(es, f, arg, NULL));
    }

    static managed<task> create_on_pool(ABT_pool p, void(*f)(void*), void* arg) {
        ABT_task t;
        TL_TASK_ASSERT(ABT_task_create(p, f, arg, &t));
        return managed<task>(t);
    }
    
    static void create_on_pool(ABT_pool p, void(*f)(void*), void* arg, const anonymous&) {
        TL_TASK_ASSERT(ABT_task_create(p, f, arg, NULL));
    } 

    void destroy() {
        if(m_task != ABT_TASK_NULL)
            ABT_task_free(&m_task);
    }

    public:

    /**
     * @brief Native handle type.
     */
    typedef ABT_task native_handle_type;

    /**
     * @brief Default constructor.
     */
    task()
    : m_task(ABT_TASK_NULL) {}

    /**
     * @brief Copy constructor.
     */
    task(const task& other) = default;

    /**
     * @brief Copy-assignment operator.
     */
    task& operator=(const task& other) = default;

    /**
     * @brief Move-assignment operator. The object
     * moved from will be invalidated if different from
     * the object moved to.
     */
    task& operator=(task&& other) {
        if(this == &other) return *this;
        m_task = other.m_task;
        other.m_task = ABT_TASK_NULL;
        return *this;
    }

    /**
     * @brief Move constructor. The object moved from will
     * be invalidated.
     */
    task(task&& other) 
    : m_task(other.m_task) {
        other.m_task = ABT_TASK_NULL;
    }

    /**
     * @brief Destructor.
     */
    ~task() = default;
    
    /**
     * @brief Blocks until the task terminates.
     * Since this routine blocks, only ULTs can call this routine.
     * If tasks use this routine, the behavior is undefined.
     */
    void join() {
        TL_TASK_ASSERT(ABT_task_join(m_task));
    }

    /**
     * @brief Request the cancelation of the target task.
     */
    void cancel() {
        TL_TASK_ASSERT(ABT_task_cancel(m_task));
    }

    /**
     * @brief Get the native handle of this task.
     *
     * @return the native ABT_task handle.
     */
    ABT_task native_handle() const {
        return m_task;
    }

    /**
     * @brief Get the id of the task.
     *
     * @return the id of the task.
     */
    std::uint64_t id() const {
        std::uint64_t id;
        TL_TASK_ASSERT(ABT_task_get_id(m_task, &id));
        return id;
    }

    /**
     * @brief Get the state of the task.
     *
     * @return The state of the task.
     */
    task_state state() const {
        ABT_task_state stt;
        TL_TASK_ASSERT(ABT_task_get_state(m_task, &stt));
        return (task_state)stt;
    }

    /**
     * @brief Sets the task's migratability. By default, all tasks 
     * are migratable. If flag is true, the task becomes migratable.
     * On the other hand, if flag is false, the target tasklet 
     * becomes unmigratable.
     *
     * @param flag true to make the task migratable, false otherwise.
     */
    void set_migratable(bool flag) {
        ABT_bool b = flag ? ABT_TRUE : ABT_FALSE;
        TL_TASK_ASSERT(ABT_task_set_migratable(m_task, b));
    }

    /**
     * @brief Returns whether the task is migratable or not.
     *
     * @return true if the task can be migrated, false otherwise.
     */
    bool is_migratable() const {
        ABT_bool flag;
        TL_TASK_ASSERT(ABT_task_is_migratable(m_task, &flag));
        return flag == ABT_TRUE;
    }

    /**
     * @brief Compares the task with another task.
     *
     * @param other Task to compare against.
     *
     * @return true if the two tasks are the same, false otherwise.
     */
    bool operator==(const task& other) const {
        ABT_bool b;
        TL_TASK_ASSERT(ABT_task_equal(m_task, other.m_task, &b));
        return b == ABT_TRUE;
    }

    /**
     * @brief Returns the ES associated with the task. 
     * If the target task is not associated with any ES,
     * a null xstream is returned.
     *
     * @return the ES associated with the task.
     */
    xstream get_xstream() const;

    /**
     * @brief If the task is not running, returns the pool where it is,
     * else returns the last pool where it was (the pool from which the 
     * task was popped).
     *
     * @return the last pool that had the task.
     */
    pool get_last_pool() const;

    /**
     * @brief Get the last pool's ID of this task.
     *
     * @return the last pool's ID of this task.
     */
    int get_last_pool_id() const {
        int id;
        TL_TASK_ASSERT(ABT_task_get_last_pool_id(m_task, &id));
        return id;
    }

    /**
     * @brief Returns the handle of the calling task. 
     * If ULTs call this routine, a null task will be returned.
     *
     * @return the handle to the calling task.
     */
    static task self() {
        ABT_task t;
        TL_TASK_ASSERT(ABT_task_self(&t));
        return task(t);
    }

    /**
     * @brief Returns the id of the calling task.
     * If ULTs call this routine, the result is undefined.
     *
     * @return the id of the calling task.
     */
    static std::uint64_t self_id() {
        std::uint64_t id;
        TL_TASK_ASSERT(ABT_task_self_id(&id));
        return id;
    }
};

}

#undef TL_TASK_EXCEPTION
#undef TL_TASK_ASSERT

#endif /* end of include guard */
