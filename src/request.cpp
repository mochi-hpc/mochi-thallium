#include <thallium/engine.hpp>
#include <thallium/request.hpp>
#include <thallium/endpoint.hpp>

namespace thallium {

endpoint request::get_endpoint() const {
    const struct hg_info* info = margo_get_info(m_handle);
    hg_addr_t addr;
    margo_addr_dup(m_engine->m_mid, info->addr, &addr);
    return endpoint(*m_engine, addr);
}

}
