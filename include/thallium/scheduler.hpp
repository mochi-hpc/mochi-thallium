/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_SCHEDULER_HPP
#define __THALLIUM_SCHEDULER_HPP

#include <memory>
#include <vector>
#include <abt.h>
#include <thallium/managed.hpp>
#include <thallium/pool.hpp>
#include <thallium/exception.hpp>
#include <thallium/abt_errors.hpp>

namespace thallium {

class pool;
class xstream;

/**
 * Exception class thrown by the mutex class.
 */
class scheduler_exception : public exception {

    public:

    template<typename ... Args>
        scheduler_exception(Args&&... args)
        : exception(std::forward<Args>(args)...) {}
};

#define TL_SCHED_EXCEPTION(__fun,__ret) \
    scheduler_exception(#__fun," returned ", abt_error_get_name(__ret),\
            " (", abt_error_get_description(__ret),") in ",__FILE__,":",__LINE__)

#define TL_SCHED_ASSERT(__call) {\
    int __ret = __call; \
    if(__ret != ABT_SUCCESS) {\
        throw TL_SCHED_EXCEPTION(__call, __ret);\
    }\
}

/**
 * @brief Wrapper for Argobots' ABT_scheduler.
 */
class scheduler {

    friend class xstream;
    friend class managed<scheduler>;

    public:
    /**
     * @brief Predefined scheduler types:
     * default, basic, priority, random-work-stealing.
     */
    enum class predef : std::int32_t {
        deflt  = ABT_SCHED_DEFAULT,
        basic  = ABT_SCHED_BASIC,
        prio   = ABT_SCHED_PRIO,
        randws = ABT_SCHED_RANDWS
    };

    private:

    template<typename S, typename Salloc = std::allocator<S>>
    struct sched_def {

        private:

        static Salloc scheduler_allocator;

        public:

        static int init(ABT_sched s, ABT_sched_config) {
            auto ss = scheduler_allocator.allocate(1);
            scheduler_allocator.construct(ss,s);
            return ABT_sched_set_data(s, static_cast<void*>(ss));
        }

        static void run(ABT_sched s) {
            void* data;
            ABT_sched_get_data(s, &data);
            S* impl = static_cast<S*>(data);
            impl->run();
        }

        static int free(ABT_sched s) {
            void* data;
            ABT_sched_get_data(s, &data);
            S* impl = static_cast<S*>(data);
            impl->~S();
            scheduler_allocator.deallocate(impl,1);
        }
        
        static ABT_pool get_migr_pool(ABT_sched s) {
            void* data;
            ABT_sched_get_data(s, &data);
            S* impl = static_cast<S*>(data);
            return impl->get_migr_pool().native_handle();
        }
    };

	ABT_sched m_sched;

    protected:

    explicit scheduler(ABT_sched s)
    : m_sched(s) {}

    private:

    void destroy() {
        // XXX for now the "automatic" parameter in scheduler config
        // is not implemented so if we free things here we end up with
        // double-free corruptions
//        if(m_sched != ABT_SCHED_NULL)
//            ABT_sched_free(&m_sched);
    }

	public:

    /**
     * @brief Underlying native handle type.
     */
    typedef ABT_sched native_handle_type;

    /**
     * @brief Get the underlying native handle.
     *
     * @return the underlying native handle.
     */
    native_handle_type native_handle() const {
        return m_sched;
    }

    /**
     * @brief Creates a scheduler based on a custom class S.
     *
     * @tparam S class of scheduler implementation.
     * @tparam I Iterator type for a container of pools.
     * @param begin Beginning iterator for the container of pools.
     * @param end End iterator for the container of pools.
     * @return a managed<scheduler> object.
     */
    template<typename S, typename I>
    static managed<scheduler> create(const I& begin, const I& end) {
        std::vector<ABT_pool> pools;
        unsigned i = 0;
        for(auto it = begin; it != end; it++, i++) {
            pools.push_back(it->native_handle());
        }
        ABT_sched_def def;
        def.type          = ABT_SCHED_TYPE_ULT;
        def.init          = sched_def<S>::init;
        def.run           = sched_def<S>::run;
        def.free          = sched_def<S>::free;
        def.get_migr_pool = sched_def<S>::get_migr_pool;
        ABT_sched sched;
        TL_SCHED_ASSERT(ABT_sched_create(&def, i, pools.data(), ABT_SCHED_CONFIG_NULL, &sched));
        return managed<scheduler>(sched);
    }

    /**
     * @brief Creates a scheduler based on a custom class S.
     *
     * @tparam S class implementing the scheduler.
     * @param spd Scheduler type.
     * @param begin Beginning iterator for the container of pools.
     * @param end End iterator for the container of pools.
     * @return a managed<scheduler> object.
     */
    template<typename S>
    static managed<scheduler> create(const pool& p) {
        std::vector<ABT_pool> pools(1);
        pools[0] = p.native_handle();
        ABT_sched_def def;
        def.type          = ABT_SCHED_TYPE_ULT;
        def.init          = sched_def<S>::init;
        def.run           = sched_def<S>::run;
        def.free          = sched_def<S>::free;
        def.get_migr_pool = sched_def<S>::get_migr_pool;
        ABT_sched sched;
        TL_SCHED_ASSERT(ABT_sched_create(&def, 1, pools.data(), ABT_SCHED_CONFIG_NULL, &sched));
        return managed<scheduler>(sched);
    }

    /**
     * @brief Creates a scheduler based on a predefined scheduler type.
     *
     * @tparam I Iterator type for a container of pools.
     * @param spd Scheduler type.
     * @param begin Beginning iterator for the container of pools.
     * @param end End iterator for the container of pools.
     * @return a managed<scheduler> object.
     */
    template<typename I>
    static managed<scheduler> create(predef spd, const I& begin, const I& end) {
        std::vector<ABT_pool> pools;
        unsigned i = 0;
        for(auto it = begin; it != end; it++, i++) {
            pools.push_back(it->native_handle());
        }
        ABT_sched_predef predef = (ABT_sched_predef)spd;
        ABT_sched sched;
        TL_SCHED_ASSERT(ABT_sched_create_basic(predef, i, &pools[0], ABT_SCHED_CONFIG_NULL, &sched));
        return managed<scheduler>(sched);
    }

    /**
     * @brief Creates a scheduler based on a predefined scheduler type.
     *
     * @tparam I Iterator type for a container of pools.
     * @param spd Scheduler type.
     * @param begin Beginning iterator for the container of pools.
     * @param end End iterator for the container of pools.
     * @return a managed<scheduler> object.
     */
    static managed<scheduler> create(predef spd, const pool& p) {
        std::vector<ABT_pool> pools(1);
        pools[0] = p.native_handle();
        ABT_sched_predef predef = (ABT_sched_predef)spd;
        ABT_sched sched;
        TL_SCHED_ASSERT(ABT_sched_create_basic(predef, 1, &pools[0], ABT_SCHED_CONFIG_NULL, &sched));
        return managed<scheduler>(sched);
    }

    /**
     * @brief Copy constructor is deleted.
     */
    scheduler(const scheduler& other) = default;

    /**
     * @brief Move constructor.
     */
    scheduler(scheduler&& other)
    : m_sched(other.m_sched) {
        other.m_sched = ABT_SCHED_NULL;
    }

    /**
     * @brief Copy assignment operator is deleted.
     */
    scheduler& operator=(const scheduler& other) = default;

    /**
     * @brief Move assignment operator.
     */
    scheduler& operator=(scheduler&& other) {
        if(this == &other) return *this;
        m_sched = other.m_sched;
        other.m_sched = ABT_SCHED_NULL;
        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~scheduler() = default;

    /**
     * @brief Get the number of pools that the scheduler
     * pulls work from.
     *
     * @return the number of pools associated with the scheduler.
     */
    std::size_t num_pools() const {
        int np;
        TL_SCHED_ASSERT(ABT_sched_get_num_pools(m_sched, &np));
        return np;
    }

    /**
     * @brief Get a particular pool associated with the scheduler.
     *
     * @param index Index of the pool.
     *
     * @return a pool associated with the scheduler.
     */
    pool get_pool(int index) const;

    /**
     * @brief Get the sum of the sizes of the pool of sched.
     * The size includes the blocked and migrating ULTs.
     *
     * @return the sum of the sizes of the pool of sched.
     */
    std::size_t total_size() const {
        std::size_t s;
        TL_SCHED_ASSERT(ABT_sched_get_total_size(m_sched, &s));
        return s;
    }

    /**
     * @brief Get the sum of the sizes of the pool of sched.
     * The size does not include the blocked and migrating ULTs.
     *
     * @return the sum of the sizes of the pool of sched.
     */
    std::size_t size() const {
        std::size_t s;
        TL_SCHED_ASSERT(ABT_sched_get_size(m_sched, &s));
        return s;
    }

    /**
     * @brief Check if the scheduler needs to stop.
     * Check if there has been an exit or a finish request and 
     * if the conditions are respected (empty pool for a finish 
     * request). If we are on the primary ES, we will jump back 
     * to the main ULT, if the scheduler has nothing to do.
     * It is the user's responsibility to take proper measures 
     * to stop the scheduling loop, depending on the value given by stop.
     *
     * @return true if the scheduler has to stop.
     */
    bool has_to_stop() const {
        ABT_bool stop;
        TL_SCHED_ASSERT(ABT_sched_has_to_stop(m_sched, &stop));
        return stop == ABT_TRUE ? true : false;
    }

    /**
     * @brief Ask a scheduler to stop as soon as possible.
     * The scheduler will stop even if its pools are not empty.
     * It is the user's responsibility to ensure that the left 
     * work will be done by another scheduler.
     */
    void exit() {
        TL_SCHED_ASSERT(ABT_sched_exit(m_sched));
    }

    /**
     * @brief Ask a scheduler to finish. The scheduler will stop
     * when its pools will be empty. 
     */
    void finish() {
        TL_SCHED_ASSERT(ABT_sched_finish(m_sched));
    }
};

}

#endif /* end of include guard */
