/*
 * (C) 2026 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BULK_BUFFER_HPP
#define __THALLIUM_BULK_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <thallium/bulk.hpp>
#include <thallium/bulk_mode.hpp>

namespace thallium {

class engine;
class endpoint;
class remote_bulk;
class async_bulk_op;

template <typename A>
class bulk_buffer_pool;

/**
 * @brief A refcounted buffer registered for RDMA.
 *
 * @tparam A Allocator type (value_type must be char). Defaults to
 *           std::allocator<char>.
 *
 * bulk_buffer allocates backing memory via A and registers it as a bulk
 * handle with the local Margo instance.  Copies share the same allocation
 * via std::shared_ptr; the buffer is deregistered and freed only when the
 * last copy is destroyed.
 *
 * The class deliberately does NOT expose an implicit conversion to
 * thallium::bulk.  Such a conversion would allow callers to keep a Mercury
 * reference alive past the buffer's logical lifetime (e.g. after a pool has
 * reclaimed the slot).  All needed RDMA operations are provided directly.
 */
template <typename A = std::allocator<char>>
class bulk_buffer {

    friend class bulk_buffer_pool<A>;

    using alloc_traits = std::allocator_traits<A>;
    using pointer_type = typename alloc_traits::pointer;

    struct impl {
        thallium::bulk b;       // owns the hg_bulk_t; freed in ~bulk()
        pointer_type   data;    // backing memory, allocated by alloc
        std::size_t    size;
        A              alloc;

        impl(engine& e, std::size_t sz, bulk_mode mode, A al);
        ~impl() noexcept;

        // Non-copyable / non-movable (managed exclusively via shared_ptr)
        impl(const impl&) = delete;
        impl& operator=(const impl&) = delete;
    };

    std::shared_ptr<impl> m_impl;

    // Private constructor used by bulk_buffer_pool (custom-deleter shared_ptr).
    explicit bulk_buffer(std::shared_ptr<impl> p) noexcept
    : m_impl(std::move(p)) {}

  public:

    /**
     * @brief Default constructor — produces a null bulk_buffer (is_null() == true).
     */
    bulk_buffer() = default;

    /**
     * @brief Allocates @p size bytes via @p alloc and registers them for RDMA.
     *
     * @param e    Engine to register with.
     * @param size Number of bytes to allocate.
     * @param mode Access mode (bulk_mode::read_only, write_only, or read_write).
     * @param alloc Allocator instance.
     */
    bulk_buffer(engine& e, std::size_t size, bulk_mode mode, A alloc = A{})
    : m_impl(std::make_shared<impl>(e, size, mode, std::move(alloc))) {}

    bulk_buffer(const bulk_buffer&)            = default;
    bulk_buffer& operator=(const bulk_buffer&) = default;
    bulk_buffer(bulk_buffer&&)                 = default;
    bulk_buffer& operator=(bulk_buffer&&)      = default;
    ~bulk_buffer()                             = default;

    /**
     * @brief Whether this is a null (empty) bulk_buffer.
     */
    bool is_null() const noexcept { return !m_impl; }

    /**
     * @brief Size of the backing buffer in bytes.
     */
    std::size_t size() const noexcept {
        return m_impl ? m_impl->size : 0;
    }

    /**
     * @brief Pointer to the backing buffer.
     */
    void* data() const noexcept {
        if(!m_impl) return nullptr;
        return static_cast<void*>(m_impl->data);
    }

    /**
     * @brief Number of bulk_buffer instances sharing this buffer.
     */
    std::uint32_t use_count() const noexcept {
        return m_impl ? static_cast<std::uint32_t>(m_impl.use_count()) : 0;
    }

    /**
     * @brief Associates this buffer with an endpoint to form a remote_bulk.
     */
    remote_bulk on(const endpoint& ep) const noexcept;

    /**
     * @brief Push data from this buffer to the remote side.
     */
    std::size_t operator>>(const remote_bulk& rb) const;

    /**
     * @brief Pull data from the remote side into this buffer.
     */
    std::size_t operator<<(const remote_bulk& rb) const;

    /**
     * @brief Async push from this buffer to the remote side.
     */
    async_bulk_op push_to(const remote_bulk& rb) const;

    /**
     * @brief Async pull from the remote side into this buffer.
     */
    async_bulk_op pull_from(const remote_bulk& rb) const;
};

/**
 * @brief Pull from the remote side into @p buf (remote_bulk >> buf notation).
 */
template <typename A>
inline std::size_t operator>>(const remote_bulk& rb, const bulk_buffer<A>& buf) {
    return buf << rb;
}

/**
 * @brief Async pull from the remote side into @p buf.
 */
template <typename A>
inline async_bulk_op pull_to(const remote_bulk& rb, const bulk_buffer<A>& buf) {
    return buf.pull_from(rb);
}

} // namespace thallium

// Full definitions needed for the implementations below.
#include <thallium/engine.hpp>
#include <thallium/endpoint.hpp>
#include <thallium/remote_bulk.hpp>

namespace thallium {

template <typename A>
bulk_buffer<A>::impl::impl(engine& e, std::size_t sz, bulk_mode mode, A al)
: size(sz)
, alloc(std::move(al)) {
    data = alloc_traits::allocate(alloc, sz);
    try {
        void* raw = static_cast<void*>(data);
        std::vector<std::pair<void*, std::size_t>> segs{{raw, sz}};
        b = e.expose(segs, mode);
    } catch(...) {
        alloc_traits::deallocate(alloc, data, sz);
        data = pointer_type{};
        throw;
    }
}

template <typename A>
bulk_buffer<A>::impl::~impl() noexcept {
    // b.~bulk() is called automatically, which calls margo_bulk_free.
    if(data) alloc_traits::deallocate(alloc, data, size);
}

template <typename A>
remote_bulk bulk_buffer<A>::on(const endpoint& ep) const noexcept {
    return m_impl->b.on(ep);
}

template <typename A>
std::size_t bulk_buffer<A>::operator>>(const remote_bulk& rb) const {
    return m_impl->b >> rb;
}

template <typename A>
std::size_t bulk_buffer<A>::operator<<(const remote_bulk& rb) const {
    return m_impl->b << rb;
}

template <typename A>
async_bulk_op bulk_buffer<A>::push_to(const remote_bulk& rb) const {
    return m_impl->b.push_to(rb);
}

template <typename A>
async_bulk_op bulk_buffer<A>::pull_from(const remote_bulk& rb) const {
    return m_impl->b.pull_from(rb);
}

} // namespace thallium

#endif /* __THALLIUM_BULK_BUFFER_HPP */
