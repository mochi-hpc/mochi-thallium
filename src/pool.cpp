#include <thallium/pool.hpp>
#include <thallium/scheduler.hpp>

namespace thallium {

void pool::add_sched(const scheduler& sched) {
    int ret = ABT_pool_add_sched(m_pool, sched.native_handle());
    if(ret != ABT_SUCCESS) {
        throw pool_exception("ABT_pool_add_sched(m_pool, sched.native_handle()) returned ", abt_error_get_name(ret),
                            " (", abt_error_get_description(ret),") in ",__FILE__,":",__LINE__);
    }
}

}
