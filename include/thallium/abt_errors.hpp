/*
 * Copyright (c) 2017 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __THALLIUM_ABT_ERRORS_HPP
#define __THALLIUM_ABT_ERRORS_HPP
#include <abt.h>

namespace thallium {

inline const char* abt_error_get_name(int err) {
    static const char* const abt_errors_names[] = { 
        "ABT_SUCCESS",
        "ABT_ERR_UNINITIALIZED",
        "ABT_ERR_MEM",
        "ABT_ERR_OTHER",
        "ABT_ERR_INV_XSTREAM",
        "ABT_ERR_INV_XSTREAM_RANK",
        "ABT_ERR_INV_XSTREAM_BARRIER",
        "ABT_ERR_INV_SCHED",
        "ABT_ERR_INV_SCHED_KIND",
        "ABT_ERR_INV_SCHED_PREDEF",
        "ABT_ERR_INV_SCHED_TYPE",
        "ABT_ERR_INV_SCHED_CONFIG",
        "ABT_ERR_INV_POOL",
        "ABT_ERR_INV_POOL_KIND",
        "ABT_ERR_INV_POOL_ACCESS",
        "ABT_ERR_INV_UNIT",
        "ABT_ERR_INV_THREAD",
        "ABT_ERR_INV_THREAD_ATTR",
        "ABT_ERR_INV_TASK",
        "ABT_ERR_INV_KEY",
        "ABT_ERR_INV_MUTEX",
        "ABT_ERR_INV_MUTEX_ATTR",
        "ABT_ERR_INV_COND",
        "ABT_ERR_INV_RWLOCK",
        "ABT_ERR_INV_EVENTUAL",
        "ABT_ERR_INV_FUTURE",
        "ABT_ERR_INV_BARRIER",
        "ABT_ERR_INV_TIMER",
        "ABT_ERR_INV_QUERY_KIND",
        "ABT_ERR_XSTREAM",
        "ABT_ERR_XSTREAM_STATE",
        "ABT_ERR_XSTREAM_BARRIER",
        "ABT_ERR_SCHED",
        "ABT_ERR_SCHED_CONFIG",
        "ABT_ERR_POOL",
        "ABT_ERR_UNIT",
        "ABT_ERR_THREAD",
        "ABT_ERR_TASK",
        "ABT_ERR_KEY",
        "ABT_ERR_MUTEX",
        "ABT_ERR_MUTEX_LOCKED",
        "ABT_ERR_COND",
        "ABT_ERR_COND_TIMEDOUT",
        "ABT_ERR_RWLOCK",
        "ABT_ERR_EVENTUAL",
        "ABT_ERR_FUTURE",
        "ABT_ERR_BARRIER",
        "ABT_ERR_TIMER",
        "ABT_ERR_MIGRATION_TARGET",
        "ABT_ERR_MIGRATION_NA",
        "ABT_ERR_MISSING_JOIN",
        "ABT_ERR_FEATURE_NA" };
    return abt_errors_names[err];
}

inline const char* abt_error_get_description(int err) {
    static const char* const abt_error_descriptions[] = {
        "Successful return code",
        "Uninitialized",
        "Memory allocation failure",
        "Other error",
        "Invalid ES",
        "Invalid ES rank",
        "Invalid ES barrier",
        "Invalid scheduler",
        "Invalid scheduler kind",
        "Invalid predefined scheduler",
        "Invalid scheduler type",
        "Invalid scheduler config",
        "Invalid pool",
        "Invalid pool kind",
        "Invalid pool access mode",
        "Invalid scheduling unit",
        "Invalid ULT",
        "Invalid ULT attribute",
        "Invalid tasklet",
        "Invalid key",
        "Invalid mutex",
        "Invalid mutex attribute",
        "Invalid condition variable",
        "Invalid rw lock",
        "Invalid eventual",
        "Invalid future",
        "Invalid barrier",
        "Invalid timer",
        "Invalid query kind",
        "ES-related error",
        "ES state error",
        "ES barrier-related error",
        "Scheduler-related error",
        "Scheduler config error",
        "Pool-related error",
        "Scheduling unit-related error",
        "ULT-related error",
        "Task-related error",
        "Key-related error",
        "Mutex-related error",
        "Return value when mutex is locked",
        "Condition-related error",
        "Return value when cond is timed out",
        "Rwlock-related error",
        "Eventual-related error",
        "Future-related error",
        "Barrier-related error",
        "Timer-related error",
        "Migration target error",
        "Migration not available",
        "An ES or more did not join",
        "Feature not available"};
    return abt_error_descriptions[err];
}

} // namespace thallium

#endif
