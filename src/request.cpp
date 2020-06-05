#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>
#include <thallium/request.hpp>

namespace thallium {

endpoint request::get_endpoint() const {
    const struct hg_info* info = margo_get_info(m_handle);
    hg_addr_t             addr;
    auto engine_impl = m_engine_impl.lock();
    if(!engine_impl) throw exception("Invalid engine");
    hg_return_t ret = margo_addr_dup(engine_impl->m_mid, info->addr, &addr);
    MARGO_ASSERT(ret, margo_addr_dup);
    return endpoint(m_engine_impl, addr);
}

} // namespace thallium
