/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_REMOTE_BULK_HPP
#define __THALLIUM_REMOTE_BULK_HPP

#include <cstdint>
#include <margo.h>
#include <string>
#include <thallium/bulk.hpp>
#include <thallium/margo_instance_ref.hpp>
#include <vector>

namespace thallium {

/**
 * @brief A remote_bulk object represents a bulk_segment object
 * that has been associated with an endpoint and is ready for
 * RDMA operations.
 */
class remote_bulk {
    friend class bulk;
    friend class bulk_segment;

  private:
    bulk_segment m_segment;
    endpoint     m_endpoint;

    /**
     * @brief Constructor. Made private since remote_bulk objects
     * are created by the function bulk::on() or bulk_segment::on()
     * functions.
     *
     * @param b bulk_segment that created the remote_bulk object.
     * @param ep endpoint on which the bulk_segment is.
     */
    remote_bulk(bulk_segment b, endpoint ep)
    : m_segment(std::move(b))
    , m_endpoint(std::move(ep)) {}

  public:
    /**
     * @brief Default copy constructor.
     */
    remote_bulk(const remote_bulk&) = default;

    /**
     * @brief Default move constructor.
     */
    remote_bulk(remote_bulk&&) = default;

    /**
     * @brief Default copy-assignment operator.
     */
    remote_bulk& operator=(const remote_bulk&) = default;

    /**
     * @brief Default move-assignment operator.
     */
    remote_bulk& operator=(remote_bulk&&) = default;

    /**
     * @brief Performs a pull operation from the remote_bulk
     * (left operand) to the destination bulk (right operand).
     * The destination must be local. If the sizes don't match,
     * the smallest size is picked.
     *
     * @param dest Local bulk segment on which to pull the data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator>>(const bulk_segment& dest) const;

    /**
     * @brief Pulls data from the left operand (remote_bulk)
     * to the right operand (bulk_segment). If the size of the
     * segments don't match, the smallest size is used.
     * This operation is non-blocking, returning an async_bulk_op
     * object that the caller can wait on.
     *
     * @param dest bulk_segment object towards which to pull data.
     *
     * @return an async_bulk_op object.
     */
    async_bulk_op pull_to(const bulk_segment& dest) const;

    /**
     * @brief Performs a push operation from the source bulk
     * (right operand) to the remote_bulk (left operand).
     * The source must be local. If the sizes don't match,
     * the smallest size is picked.
     *
     * @param src Local bulk segment from which to push the data.
     *
     * @return the size of data transfered.
     */
    std::size_t operator<<(const bulk_segment& src) const;

    /**
     * @brief Pushes data from the right operand (bulk_segment)
     * to the left operand (remote_bulk). If the size of the
     * segments don't match, the smallest size is used.
     * This operation is non-blocking, returning an async_bulk_op
     * object that the caller can wait on.
     *
     * @param src Local bulk segment from which to push data.
     *
     * @return an async_bulk_op object.
     */
    async_bulk_op push_from(const bulk_segment& src) const;

    /**
     * @brief Creates a bulk_segment object by selecting a given portion
     * of the bulk object given an offset and a size.
     *
     * @param offset Offset at which the segment starts.
     * @param size Size of the segment.
     *
     * @return a bulk_segment object.
     */
    remote_bulk select(std::size_t offset, std::size_t size) const {
        return remote_bulk(m_segment.select(offset, size), m_endpoint);
    }

    /**
     * @see remote_bulk::select
     */
    inline remote_bulk operator()(std::size_t offset, std::size_t size) const {
        return select(offset, size);
    }
};

} // namespace thallium

#include <thallium/engine.hpp>

namespace thallium {

inline std::size_t remote_bulk::operator>>(const bulk_segment& dest) const {

    margo_instance_id mid           = m_endpoint.m_mid;
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

inline async_bulk_op remote_bulk::pull_to(const bulk_segment& dest) const {

    margo_instance_id mid           = m_endpoint.m_mid;
    hg_bulk_op_t      op            = HG_BULK_PULL;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = dest.m_bulk.m_bulk;
    size_t            local_offset  = dest.m_offset;
    size_t            size          = dest.m_size;
    margo_request     req           = MARGO_REQUEST_NULL;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_itransfer(mid, op, origin_addr, origin_handle, origin_offset,
                             local_handle, local_offset, size, &req);
    MARGO_ASSERT(ret, margo_bulk_itransfer);

    return async_bulk_op{size, req};
}

inline std::size_t remote_bulk::operator<<(const bulk_segment& src) const {

    margo_instance_id mid           = m_endpoint.m_mid;
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

inline async_bulk_op remote_bulk::push_from(const bulk_segment& src) const {

    margo_instance_id mid           = m_endpoint.m_mid;
    hg_bulk_op_t      op            = HG_BULK_PUSH;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = src.m_bulk.m_bulk;
    size_t            local_offset  = src.m_offset;
    size_t            size          = src.m_size;
    margo_request     req           = MARGO_REQUEST_NULL;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_itransfer(mid, op, origin_addr, origin_handle, origin_offset,
                             local_handle, local_offset, size, &req);
    MARGO_ASSERT(ret, margo_bulk_itransfer);

    return async_bulk_op{size, req};
}

} // namespace thallium

#endif
