#include "thallium/scheduler.hpp"
#include "thallium/pool.hpp"

namespace thallium {

pool scheduler::get_pool(int index) const {
    ABT_pool p;
    int      ret = ABT_sched_get_pools(m_sched, 1, index, &p);
    if(ret != ABT_SUCCESS) {
        throw scheduler_exception(
            "ABT_sched_get_pools(m_sched, 1, index, &p) returned ",
            abt_error_get_name(ret), " (", abt_error_get_description(ret),
            ") in ", __FILE__, ":", __LINE__);
    }
    return pool(p);
}

} // namespace thallium
