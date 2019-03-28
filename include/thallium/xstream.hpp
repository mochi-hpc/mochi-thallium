/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_XSTREAM_HPP
#define __THALLIUM_XSTREAM_HPP

#include <memory>
#include <vector>
#include <abt.h>

#include <thallium/anonymous.hpp>
#include <thallium/scheduler.hpp>
#include <thallium/thread.hpp>
#include <thallium/pool.hpp>
#include <thallium/managed.hpp>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>

namespace thallium {

/**
 * Exception class thrown by the task class.
 */
class xstream_exception : public exception {

    public:

        template<typename ... Args>
        xstream_exception(Args&&... args)
        : exception(std::forward<Args>(args)...) {}
};

#define TL_ES_EXCEPTION(__fun,__ret) \
    xstream_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__);

#define TL_ES_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_ES_EXCEPTION(__call, __ret);\
    }\
}

/**
 * @brief The xstream_state enum represents
 * the different states an ES can be in.
 */
enum class xstream_state : std::int32_t {
    created    = ABT_XSTREAM_STATE_CREATED,
    ready      = ABT_XSTREAM_STATE_READY,
    running    = ABT_XSTREAM_STATE_RUNNING,
    terminated = ABT_XSTREAM_STATE_TERMINATED
};

/**
 * @brief Wrapper for Argobots' ABT_xstream.
 */
class xstream {

    friend class task;
    friend class managed<xstream>;

    ABT_xstream m_xstream;

    xstream(ABT_xstream es)
    : m_xstream(es) {}

    static void forward_work_unit(void* fp) {
        auto f = static_cast<std::function<void(void)>*>(fp);
        (*f)();
        delete f;
    }

    void destroy() {
        if(m_xstream != ABT_XSTREAM_NULL)
            ABT_xstream_free(&m_xstream);
    }

	public:

    /**
     * @brief Native handle type.
     */
    typedef ABT_xstream native_handle_type;

    /**
     * @brief Get the native handle.
     *
     * @return the native handle.
     */
    native_handle_type native_handle() const {
        return m_xstream;
    }

    /**
     * @brief Creates a managed xstream object with a 
     * default scheduler and private pool.
     *
     * @return a managed<xstream> object.
     */
    static managed<xstream> create() {
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_create(ABT_SCHED_NULL, &es));
        return managed<xstream>(es);
    }

    /**
     * @brief Creates a managed xstream with a predefined scheduler and
     * a given pool.
     *
     * @param spd predefined scheduler type.
     * @param p pool for the ES's scheduler to use.
     *
     * @return a managed<xstream> object.
     */
    static managed<xstream> create(scheduler::predef spd, const pool& p) {
        ABT_pool thePool = p.native_handle();
        ABT_sched_predef predef = (ABT_sched_predef)spd;
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_create_basic(predef, 1, &thePool, ABT_SCHED_CONFIG_NULL, &es));
        return managed<xstream>(es);
    }

    /**
     * @brief Creates an xstream with a predefined scheduler and a set of pools.
     *
     * @tparam I Type of iterator of the container that contains the pools.
     * @param spd scheduler predefinition
     * @param begin beginning iterator for the set of pools to use
     * @param end end iterator for the set of pools to use
     *
     * @return a managed<xstream> object.
     */
    template<typename I>
    static managed<xstream> create(scheduler::predef spd, const I& begin, const I& end) {
        std::vector<ABT_pool> pools;
        unsigned i = 0;
        for(auto it = begin; it != end; it++, i++) {
            pools.push_back(it->native_handle());
        }
        ABT_sched_predef predef = (ABT_sched_predef)spd;
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_create_basic(predef, i, &pools[0], ABT_SCHED_CONFIG_NULL, &es));
        return managed<xstream>(es);
    }

    /**
     * @brief Creates a managed xstream using the provided scheduler.
     *
     * @param sched Scheduler that the xstream should use.
     *
     * @return a managed<xstream> object.
     */
    static managed<xstream> create(const scheduler& sched) {
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_create(sched.native_handle(), &es));
        return managed<xstream>(es);
    }

    /**
     * @brief Creates a managed xstream using the provided scheduler,
     * and assign it the given rank.
     *
     * @param sched Scheduler to be used by the xstream.
     * @param rank rank of the ES.
     *
     * @return a managed<xstream> object.
     */
    static managed<xstream> create(const scheduler& sched, int rank) {
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_create_with_rank(sched.native_handle(), rank, &es));
        return managed<xstream>(es);
    }

    /**
     * @brief Default constructor. Does NOT create an ES, but a null handle.
     */
    xstream() : m_xstream(ABT_XSTREAM_NULL) {}

    /**
     * @brief Copy-constructor.
     */
    xstream(const xstream& other) = default;

    /**
     * @brief Move-constructor. Will invalidate
     * the moved-from xstream instance.
     */
    xstream(xstream&& other) 
    : m_xstream(other.m_xstream) {
        other.m_xstream = ABT_XSTREAM_NULL;
    }

    /**
     * @brief Copy-assignment operator.
     */
    xstream& operator=(const xstream& other) = default;

    /**
     * @brief Move-assignment operator.
     */
    xstream& operator=(xstream&& other) {
        if(&other == this) return *this;
        m_xstream = other.m_xstream;
        other.m_xstream = ABT_XSTREAM_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~xstream() = default;

    /**
     * @brief Starts the xstream if it has not been started.
     * That is, this routine is effective only when the state
     * of the ES is CREATED or READY, and once this routine returns,
     * the ES's state becomes RUNNING.
     */
    void start() {
        TL_ES_ASSERT(ABT_xstream_start(m_xstream));
    }

    /**
     * @brief Wait for xstream to terminate.
     *
     * The xstream cannot be the same as the 
     * xstream associated with calling thread. 
     * If they are identical, this routine returns immediately without 
     * waiting for the xstream's termination.
     */
    void join() {
        TL_ES_ASSERT(ABT_xstream_join(m_xstream));
    }

    /**
     * @brief Request the cancelation of the target ES.
     */
    void cancel() {
        TL_ES_ASSERT(ABT_xstream_cancel(m_xstream));
    }

    /**
     * @brief Get the rank of the ES.
     *
     * @return the rank of the ES.
     */
    int get_rank() const {
        int r;
        TL_ES_ASSERT(ABT_xstream_get_rank(m_xstream, &r));
        return r;
    }

    /**
     * @brief Set the rank of the ES.
     *
     * @param r rank to set the ES to.
     */
    void set_rank(int r) {
        TL_ES_ASSERT(ABT_xstream_set_rank(m_xstream, r));
    }

    /**
     * @brief Check whether this ES if the primary ES.
     *
     * @return true if the ES is the primary one, false otherwise.
     */
    bool is_primary() const {
        ABT_bool flag;
        TL_ES_ASSERT(ABT_xstream_is_primary(m_xstream, &flag));
        return flag == ABT_TRUE;
    }

    /**
     * @brief Get the state of the ES.
     *
     * @return the state of the ES.
     */
    xstream_state state() const {
        ABT_xstream_state state;
        TL_ES_ASSERT(ABT_xstream_get_state(m_xstream, &state));
        return (xstream_state)state;
    }

    /**
     * @brief Get the id of the CPU to which this xstream is bound.
     *
     * @return the id of the CPU to which this xstream is bound.
     */
    int get_cpubind() const {
        int cpuid;
        TL_ES_ASSERT(ABT_xstream_get_cpubind(m_xstream, &cpuid));
        return cpuid;
    }

    /**
     * @brief Set the CPU affinity of the ES.
     * This method binds the target ES xstream to the target 
     * CPU whose ID is cpuid. Here, the CPU ID corresponds to 
     * the processor index used by OS.
     *
     * @param cpuid CPU ID
     */
    void set_cpubind(int cpuid) {
        TL_ES_ASSERT(ABT_xstream_set_cpubind(m_xstream, cpuid));
    }

    /**
     * @brief Get the CPU affinity for the target ES in the form
     * of a vector of CPU IDs associated with the ES.
     *
     * @return a vector of CPU IDs.
     */
    std::vector<int> get_affinity() const {
        int num_cpus;
        TL_ES_ASSERT(ABT_xstream_get_affinity(m_xstream, 0, NULL, &num_cpus));
        std::vector<int> cpuset(num_cpus);
        TL_ES_ASSERT(ABT_xstream_get_affinity(m_xstream, num_cpus, &(cpuset[0]), &num_cpus));
        return cpuset;
    }

    /**
     * @brief Sets the CPU affinity of the target ES by passing
     * iterators to a set of CPU ID values.
     *
     * @tparam I type of iterator.
     * @param begin start iterator of CPU IDs.
     * @param end end iterator of CPU ID.
     */
    template<typename I>
    void set_affinity(const I& begin, const I& end) {
        std::vector<int> cpuset(begin, end);
        TL_ES_ASSERT(ABT_xstream_set_affinity(m_xstream, cpuset.size(), &(cpuset[0])));
    }

    /**
     * @brief Compares two ES.
     *
     * @return true if the ES are the same, false otherwise. 
     */
    bool operator==(const xstream& other) const {
        ABT_bool result;
        TL_ES_ASSERT(ABT_xstream_equal(m_xstream, other.native_handle(), &result));
        return result == ABT_TRUE;
    }

    /**
     * @brief Checks whether the ES is a null handle.
     *
     * @return true if the ES is null.
     */
    bool is_null() const {
        return m_xstream == ABT_XSTREAM_NULL;
    }

    /**
     * @brief Checks whether the ES is not null.
     *
     * @return true if the ES is not null, false otherwise.
     */
    operator bool() const {
        return !is_null();
    }

    /**
     * @brief Sets the scheduler that the ES should use.
     *
     * @param s Scheduler that the ES should use.
     */
    void set_main_sched(const scheduler& s) {
        TL_ES_ASSERT(ABT_xstream_set_main_sched(m_xstream, s.native_handle()));
    }

    /**
     * @brief Sets the scheduler that the ES schould use, using
     * a scheduler predefinition and a set of pools that this scheduler should use.
     *
     * @tparam I type of iterator for the container of pools.
     * @param s scheduler predefinition.
     * @param begin starting iterator of the list of pools.
     * @param end end iterator of the list of pools.
     */
    template<typename I>
    void set_main_sched(const scheduler::predef& s, const I& begin, const I& end) {
        std::vector<ABT_pool> pools;
        unsigned i = 0;
        for(auto it = begin; it != end; it++, i++) {
            pools.push_back(it->native_handle());
        }
        ABT_sched_predef predef = (ABT_sched_predef)s;
        TL_ES_ASSERT(ABT_xstream_set_main_sched_basic(m_xstream, predef, i, &pools[0]));
    }

    /**
     * @brief Get the main scheduler used by the xstream.
     *
     * @return the main scheduler used by the xstream.
     */
    scheduler get_main_sched() const {
        ABT_sched s;
        TL_ES_ASSERT(ABT_xstream_get_main_sched(m_xstream, &s));
        return scheduler(s);
    }

    /**
     * @brief Get up to max_pools pools associated with the ES's scheduler.
     * If max_pools is not provided, this method retrieves all the pools.
     *
     * @param max_pools maximum number of pools to retrieve.
     *
     * @return a vector of pools associated with the ES.
     */
    std::vector<pool> get_main_pools(int max_pools=-1) const {
        std::vector<ABT_pool> pools;
        std::vector<pool> result;
        if(max_pools == 0) return result;

        ABT_sched sched;
        TL_ES_ASSERT(ABT_xstream_get_main_sched(m_xstream, &sched));
        int num_pools;
        TL_ES_ASSERT(ABT_sched_get_num_pools(sched, &num_pools));
        pools.resize(num_pools);
        if(num_pools > max_pools && max_pools != -1)
            num_pools = max_pools;
        TL_ES_ASSERT(ABT_sched_get_pools(sched, num_pools, 0, &pools[0]));
        for(auto p : pools) {
            result.push_back(pool(p));
        }
        return result;
    }

    /**
     * @brief Create a thread running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a thread. Must have operator()().
     * @param f Function to run as a thread.
     *
     * @return a managed<thread> object managing the created thread.
     */
    template<typename F>
    managed<thread> make_thread(F&& f) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return thread::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp));
    }

    template<typename F>
    void make_thread(F&& f, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        thread::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp), a);
    }

    /**
     * @brief Create a thread running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a thread. Must have operator()().
     * @param f Function to run as a thread.
     * @param attr Thread attributes.
     *
     * @return a managed<thread> object managing the created thread.
     */
    template<typename F>
    managed<thread> make_thread(F&& f, const thread::attribute& attr) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return thread::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp), attr);
    }

    template<typename F>
    void make_thread(F&& f, const thread::attribute& attr, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        thread::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp), attr, a);
    }

    /**
     * @brief Create a task running the specified function and push it
     * into the pool.
     *
     * @tparam F type of function to run as a task. Must have operator()().
     * @param f Function to run as a task.
     *
     * @return a managed<task> object managing the create task.
     */
    template<typename F>
    managed<task> make_task(F&& f) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        return task::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp));
    }

    template<typename F>
    void make_task(F&& f, const anonymous& a) {
        auto fp = new std::function<void(void)>(std::forward<F>(f));
        task::create_on_xstream(m_xstream, forward_work_unit, static_cast<void*>(fp), a);
    }

    /**
     * @brief Terminate the ES associated with the calling ULT.
     */
    static void exit() {
        TL_ES_ASSERT(ABT_xstream_exit());
    }

    /**
     * @brief Return the ES handle associated with the caller 
     * work unit. 
     *
     * @return the ES handle associated with the caller work unit.
     */
    static xstream self() {
        ABT_xstream es;
        TL_ES_ASSERT(ABT_xstream_self(&es));
        return xstream(es);
    }

    /**
     * @brief Return the rank of the ES associated with the caller
     * work unit.
     *
     * @return the rank of the ES associated with the caller work unit.
     */
    static int self_rank() {
        int r;
        TL_ES_ASSERT(ABT_xstream_self_rank(&r));
        return r;
    }

    /**
     * @brief Returns the number of running ES.
     *
     * @return the number of running ES.
     */
    static int num() {
        int n;
        TL_ES_ASSERT(ABT_xstream_get_num(&n));
        return n;
    }

    /**
     * @brief Check the events and process them.
     *
     * This function must be called by a scheduler periodically.
     * Therefore, a user will use it on his own defined scheduler.
     *
     * @param s scheduler requesting to check for events.
     */
    static void check_events(const scheduler& s) {
        TL_ES_ASSERT(ABT_xstream_check_events(s.native_handle()));
    }
};

}

#undef TL_ES_EXCEPTION
#undef TL_ES_ASSERT

#endif /* end of include guard */
