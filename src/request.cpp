#include <thallium/engine.hpp>
#include <thallium/request.hpp>
#include <thallium/endpoint.hpp>

namespace thallium {

endpoint request::get_endpoint() const {
    const struct hg_info* info = margo_get_info(m_handle);
    hg_addr_t addr;
    hg_return_t ret = margo_addr_dup(m_engine->m_mid, info->addr, &addr);
    MARGO_ASSERT(ret, margo_addr_dup);
    return endpoint(*m_engine, addr);
}

}
