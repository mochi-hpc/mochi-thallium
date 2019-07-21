/*
 * (C) 2017 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <string>
#include <margo.h>
#include <thallium/remote_procedure.hpp>
#include <thallium/engine.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/bulk.hpp>

namespace thallium {

endpoint engine::lookup(const std::string& address) const {
    hg_addr_t addr;
    hg_return_t ret = margo_addr_lookup(m_mid, address.c_str(), &addr);
    MARGO_ASSERT(ret, margo_addr_lookup);
    return endpoint(const_cast<engine&>(*this), addr);
}

endpoint engine::self() const {
    hg_addr_t self_addr;
    hg_return_t ret = margo_addr_self(m_mid, &self_addr);
    MARGO_ASSERT(ret, margo_addr_self);
    return endpoint(const_cast<engine&>(*this), self_addr);
}

remote_procedure engine::define(const std::string& name) {
    hg_bool_t flag;
    hg_id_t id;
    margo_registered_name(m_mid, name.c_str(), &id, &flag);
    if(flag == HG_FALSE) {
        id = margo_register_name(m_mid, name.c_str(),
                    process_buffer,
                    process_buffer,
                    nullptr);
    }
    return remote_procedure(*this, id);
}

bulk engine::expose(const std::vector<std::pair<void*,size_t>>& segments, bulk_mode flag) {
    hg_bulk_t handle;
    hg_uint32_t count = segments.size();
    std::vector<void*> buf_ptrs(count);
    std::vector<hg_size_t> buf_sizes(count);
    for(unsigned i=0; i < segments.size(); i++) {
        buf_ptrs[i]  = segments[i].first;
        buf_sizes[i] = segments[i].second;
    }
    hg_return_t ret = margo_bulk_create(m_mid, count, &buf_ptrs[0], &buf_sizes[0], (hg_uint32_t)flag, &handle);
    MARGO_ASSERT(ret, margo_bulk_create);
    return bulk(*this, handle, true);
}

void engine::shutdown_remote_engine(const endpoint& ep) const {
    int ret = margo_shutdown_remote_instance(m_mid, ep.m_addr);
    hg_return_t r = ret == 0 ? HG_SUCCESS : HG_OTHER_ERROR;
    MARGO_ASSERT(r, margo_shutdown_remote_instance);
}

void engine::enable_remote_shutdown() {
    margo_enable_remote_shutdown(m_mid);
}

remote_procedure engine::define(const std::string& name,
        const std::function<void(const request&)>& fun,
        uint16_t provider_id, const pool& p) {

    hg_id_t id = margo_provider_register_name(m_mid, name.c_str(),
            process_buffer,
            process_buffer,
            rpc_callback<rpc_t, false>,
            provider_id,
            p.native_handle());

    m_rpcs[id] = [fun](const request& r, const buffer& b) {
        fun(r);
    };

    rpc_callback_data* cb_data = new rpc_callback_data;
    cb_data->m_engine   = this;
    cb_data->m_function = void_cast(&m_rpcs[id]);

    hg_return_t ret = margo_register_data(m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(*this, id);
}

}

