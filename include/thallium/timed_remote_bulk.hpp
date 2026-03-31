/*
 * (C) 2017 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_TIMED_REMOTE_BULK_HPP
#define __THALLIUM_TIMED_REMOTE_BULK_HPP

#include <chrono>
#include <margo.h>
#include <thallium/remote_bulk.hpp>
#include <thallium/timeout.hpp>

namespace thallium {

/**
 * @brief A timed_remote_bulk is a proxy around a remote_bulk that performs
 * RDMA transfers subject to a millisecond deadline. It is created by calling
 * remote_bulk::timed().
 *
 * Synchronous operations (operator>> and operator<<) throw tl::timeout if the
 * deadline expires before the transfer completes. Asynchronous operations
 * (pull_to and push_from) start the transfer and return an async_bulk_op
 * whose wait() throws tl::timeout if the deadline expires.
 */
class timed_remote_bulk {

    friend class remote_bulk;

    remote_bulk m_remote_bulk;
    double      m_timeout_ms;

    timed_remote_bulk(const remote_bulk& rb, double timeout_ms)
    : m_remote_bulk(rb)
    , m_timeout_ms(timeout_ms) {}

  public:

    timed_remote_bulk(const timed_remote_bulk&) = default;
    timed_remote_bulk(timed_remote_bulk&&)      = default;

    /**
     * @brief Synchronous timed pull: transfers data from the remote bulk
     * into the local dest segment. Throws tl::timeout if the deadline
     * expires before the transfer completes.
     *
     * @param dest Local bulk segment into which data is pulled.
     *
     * @return the number of bytes transferred.
     */
    std::size_t operator>>(const bulk_segment& dest) const {
        return m_remote_bulk.transfer_timed(dest, HG_BULK_PULL, m_timeout_ms);
    }

    /**
     * @brief Synchronous timed push: transfers data from the local src
     * segment into the remote bulk. Throws tl::timeout if the deadline
     * expires before the transfer completes.
     *
     * @param src Local bulk segment from which data is pushed.
     *
     * @return the number of bytes transferred.
     */
    std::size_t operator<<(const bulk_segment& src) const {
        return m_remote_bulk.transfer_timed(src, HG_BULK_PUSH, m_timeout_ms);
    }

    /**
     * @brief Asynchronous timed pull: initiates a non-blocking transfer
     * from the remote bulk into dest. The returned async_bulk_op::wait()
     * throws tl::timeout if the deadline expires.
     *
     * @param dest Local bulk segment into which data is pulled.
     *
     * @return an async_bulk_op tracking the in-flight transfer.
     */
    async_bulk_op pull_to(const bulk_segment& dest) const {
        return m_remote_bulk.itransfer_timed(dest, HG_BULK_PULL, m_timeout_ms);
    }

    /**
     * @brief Asynchronous timed push: initiates a non-blocking transfer
     * from src into the remote bulk. The returned async_bulk_op::wait()
     * throws tl::timeout if the deadline expires.
     *
     * @param src Local bulk segment from which data is pushed.
     *
     * @return an async_bulk_op tracking the in-flight transfer.
     */
    async_bulk_op push_from(const bulk_segment& src) const {
        return m_remote_bulk.itransfer_timed(src, HG_BULK_PUSH, m_timeout_ms);
    }
};

} // namespace thallium

#include <thallium/engine.hpp>

namespace thallium {

inline std::size_t remote_bulk::transfer_timed(const bulk_segment& local,
                                                hg_bulk_op_t        op,
                                                double              timeout_ms) const {
    margo_instance_id mid           = m_endpoint.m_mid;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = local.m_bulk.m_bulk;
    size_t            local_offset  = local.m_offset;
    size_t            size          = local.m_size;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_transfer_timed(mid, op, origin_addr, origin_handle, origin_offset,
                                  local_handle, local_offset, size, timeout_ms);
    if(ret == HG_TIMEOUT) throw timeout{};
    MARGO_ASSERT(ret, margo_bulk_transfer_timed);
    return size;
}

inline async_bulk_op remote_bulk::itransfer_timed(const bulk_segment& local,
                                                   hg_bulk_op_t        op,
                                                   double              timeout_ms) const {
    margo_instance_id mid           = m_endpoint.m_mid;
    hg_addr_t         origin_addr   = m_endpoint.m_addr;
    hg_bulk_t         origin_handle = m_segment.m_bulk.m_bulk;
    size_t            origin_offset = m_segment.m_offset;
    hg_bulk_t         local_handle  = local.m_bulk.m_bulk;
    size_t            local_offset  = local.m_offset;
    size_t            size          = local.m_size;
    margo_request     req           = MARGO_REQUEST_NULL;

    if(size > m_segment.m_size)
        size = m_segment.m_size;

    hg_return_t ret =
        margo_bulk_itransfer_timed(mid, op, origin_addr, origin_handle, origin_offset,
                                   local_handle, local_offset, size, timeout_ms, &req);
    if(ret == HG_TIMEOUT) throw timeout{};
    MARGO_ASSERT(ret, margo_bulk_itransfer_timed);
    return async_bulk_op{size, req};
}

inline timed_remote_bulk remote_bulk::timed(double timeout_ms) const noexcept {
    return timed_remote_bulk(*this, timeout_ms);
}

template<typename Rep, typename Period>
inline timed_remote_bulk remote_bulk::timed(
        const std::chrono::duration<Rep,Period>& d) const noexcept {
    using ms = std::chrono::duration<double, std::milli>;
    return timed(std::chrono::duration_cast<ms>(d).count());
}

} // namespace thallium

#endif
