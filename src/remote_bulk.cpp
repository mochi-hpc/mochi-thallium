/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <thallium/bulk.hpp>
#include <thallium/engine.hpp>
#include <thallium/remote_bulk.hpp>

namespace thallium {

std::size_t remote_bulk::operator>>(const bulk::bulk_segment& dest) const {

    margo_instance_id mid           = m_segment.m_bulk.m_engine->m_mid;
    hg_bulk_op_t      op            = HG_BULK_PULL;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = dest.m_bulk.m_bulk;
    size_t            local_offset  = dest.m_offset;
    size_t            size          = dest.m_size;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_transfer(mid, op, origin_addr, origin_handle, origin_offset,
                            local_handle, local_offset, size);
    MARGO_ASSERT(ret, margo_bulk_transfer);

    return size;
}

std::size_t remote_bulk::operator<<(const bulk::bulk_segment& src) const {

    margo_instance_id mid           = m_segment.m_bulk.m_engine->m_mid;
    hg_bulk_op_t      op            = HG_BULK_PUSH;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = src.m_bulk.m_bulk;
    size_t            local_offset  = src.m_offset;
    size_t            size          = src.m_size;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_transfer(mid, op, origin_addr, origin_handle, origin_offset,
                            local_handle, local_offset, size);
    MARGO_ASSERT(ret, margo_bulk_transfer);

    return size;
}

} // namespace thallium
