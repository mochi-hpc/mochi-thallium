#include <thallium/engine.hpp>
#include <thallium/pool.hpp>
#include <thallium/scheduler.hpp>
#include <thallium/thread.hpp>
#include <thallium/xstream.hpp>

namespace thallium {

void thread::migrate_to(xstream& es) {
    ABT_thread_migrate_to_xstream(m_thread, es.native_handle());
}

void thread::migrate_to(scheduler& sched) {
    ABT_thread_migrate_to_sched(m_thread, sched.native_handle());
}

void thread::migrate_to(pool& p) {
    ABT_thread_migrate_to_pool(m_thread, p.native_handle());
}

pool thread::get_last_pool() const {
    ABT_pool p;
    ABT_thread_get_last_pool(m_thread, &p);
    return pool(p);
}

void thread::sleep(engine& eng, double ms) {
    margo_thread_sleep(eng.get_margo_instance(), ms);
}

} // namespace thallium
