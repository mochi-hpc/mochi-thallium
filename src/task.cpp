#include "thallium/task.hpp"
#include "thallium/xstream.hpp"
#include "thallium/pool.hpp"

namespace thallium {

xstream task::get_xstream() const {
    ABT_xstream es;
    int ret = ABT_task_get_xstream(m_task, &es);
    if(ret != ABT_SUCCESS) {
        throw task_exception("ABT_task_get_xstream(m_task, &es) returned ", abt_error_get_name(ret),
                " (", abt_error_get_description(ret),") in ",__FILE__,":",__LINE__);
    }
    return xstream(es);
}

pool task::get_last_pool() const {
    ABT_pool p;
    int ret = ABT_task_get_last_pool(m_task, &p);
    if(ret != ABT_SUCCESS) {
        throw task_exception("ABT_task_get_xstream(m_task, &es) returned ", abt_error_get_name(ret),
                " (", abt_error_get_description(ret),") in ",__FILE__,":",__LINE__);
    }
    return pool(p);
}

}
