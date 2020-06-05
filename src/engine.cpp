/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <margo.h>
#include <string>
#include <thallium/bulk.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/engine.hpp>
#include <thallium/remote_procedure.hpp>

namespace thallium {

/**
 * @brief Function run as a ULT when receiving an RPC.
 *
 * @param handle handle of the RPC.
 */
hg_return_t thallium_generic_rpc(hg_handle_t handle) {
    margo_instance_id mid = margo_hg_handle_get_instance(handle);
    THALLIUM_ASSERT_CONDITION(mid != 0,
            "margo_hg_handle_get_instance returned null");
    const struct hg_info* info = margo_get_info(handle);
    THALLIUM_ASSERT_CONDITION(info != nullptr,
            "margo_get_info returned null");
    void* data = margo_registered_data(mid, info->id);
    THALLIUM_ASSERT_CONDITION(data != nullptr,
            "margo_registered_data returned null");
    auto    cb_data = static_cast<engine::rpc_callback_data*>(data);
    auto&   rpc = cb_data->m_function;
    // TODO throw if m_engine_impl is invalid
    request req(cb_data->m_engine_impl, handle, false);
    rpc(req);
    margo_destroy(handle); // because of margo_ref_incr in rpc_callback
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(thallium_generic_rpc);

endpoint engine::lookup(const std::string& address) const {
    // TODO throw if m_impl is invalid
    hg_addr_t   addr;
    hg_return_t ret = margo_addr_lookup(m_impl->m_mid, address.c_str(), &addr);
    MARGO_ASSERT(ret, margo_addr_lookup);
    return endpoint(const_cast<engine&>(*this), addr);
}

endpoint engine::self() const {
    // TODO throw if m_impl is invalid
    hg_addr_t   self_addr;
    hg_return_t ret = margo_addr_self(m_impl->m_mid, &self_addr);
    MARGO_ASSERT(ret, margo_addr_self);
    return endpoint(const_cast<engine&>(*this), self_addr);
}

remote_procedure engine::define(const std::string& name) {
    hg_bool_t flag;
    hg_id_t   id;
    // TODO throw if m_impl is invalid
    margo_registered_name(m_impl->m_mid, name.c_str(), &id, &flag);
    if(flag == HG_FALSE) {
        id = MARGO_REGISTER(m_impl->m_mid, name.c_str(), meta_serialization, meta_serialization, NULL);
    }
    return remote_procedure(m_impl, id);
}

bulk engine::expose(const std::vector<std::pair<void*, size_t>>& segments,
                    bulk_mode                                    flag) {
    hg_bulk_t              handle;
    hg_uint32_t            count = segments.size();
    std::vector<void*>     buf_ptrs(count);
    std::vector<hg_size_t> buf_sizes(count);
    for(unsigned i = 0; i < segments.size(); i++) {
        buf_ptrs[i]  = segments[i].first;
        buf_sizes[i] = segments[i].second;
    }
    // TODO throw if m_impl is invalid
    hg_return_t ret = margo_bulk_create(
        m_impl->m_mid, count, &buf_ptrs[0], &buf_sizes[0],
        static_cast<hg_uint32_t>(flag), &handle);
    MARGO_ASSERT(ret, margo_bulk_create);
    return bulk(m_impl, handle, true);
}

bulk engine::wrap(hg_bulk_t blk, bool is_local) {
    hg_return_t hret = margo_bulk_ref_incr(blk);
    MARGO_ASSERT(hret, margo_bulk_ref_incr);
    // TODO throw if m_impl is invalid
    return bulk(m_impl, blk, is_local);
}

void engine::shutdown_remote_engine(const endpoint& ep) const {
    // TODO throw if m_impl is invalid
    int         ret = margo_shutdown_remote_instance(m_impl->m_mid, ep.m_addr);
    hg_return_t r   = ret == 0 ? HG_SUCCESS : HG_OTHER_ERROR;
    MARGO_ASSERT(r, margo_shutdown_remote_instance);
}

void engine::enable_remote_shutdown() {
    // TODO throw if m_impl is invalid
    margo_enable_remote_shutdown(m_impl->m_mid);
}

remote_procedure engine::define(const std::string&                         name,
                                const std::function<void(const request&)>& fun,
                                uint16_t provider_id, const pool& p) {
    // TODO throw if m_impl is invalid
    hg_id_t id = MARGO_REGISTER_PROVIDER(
        m_impl->m_mid, name.c_str(), meta_serialization, meta_serialization,
        thallium_generic_rpc, provider_id, p.native_handle());

    auto* cb_data          = new rpc_callback_data;
    cb_data->m_engine_impl = m_impl;
    cb_data->m_function    = fun;

    hg_return_t ret =
        margo_register_data(m_impl->m_mid, id, (void*)cb_data, free_rpc_callback_data);
    MARGO_ASSERT(ret, margo_register_data);

    return remote_procedure(m_impl, id);
}

} // namespace thallium
