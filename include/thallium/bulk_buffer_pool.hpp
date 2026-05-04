/*
 * (C) 2026 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __THALLIUM_BULK_BUFFER_POOL_HPP
#define __THALLIUM_BULK_BUFFER_POOL_HPP

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>
#include <thallium/bulk_buffer.hpp>
#include <thallium/engine.hpp>
#include <thallium/mutex.hpp>
#include <thallium/condition_variable.hpp>

namespace thallium {

/**
 * @brief Multi-tier pool of pre-allocated, RDMA-registered bulk_buffer objects.
 *
 * @tparam A Allocator type (same constraints as bulk_buffer<A>).
 *
 * The pool owns all bulk_buffer slots.  Callers borrow slots via get() /
 * try_get(), which return a bulk_buffer<A> whose shared_ptr carries a custom
 * deleter that returns the slot to the free list (instead of destroying it).
 *
 * Lifetime: the internal pool_state is kept alive by a shared_ptr shared
 * between the pool object itself and every outstanding lease.  Destroying
 * the bulk_buffer_pool before all leases are returned is therefore safe —
 * the pool_state (and its memory) persists until the last lease is dropped.
 */
template <typename A = std::allocator<char>>
class bulk_buffer_pool {

    using impl_type = typename bulk_buffer<A>::impl;

    // -----------------------------------------------------------------------
    // Internal per-tier state

    struct bucket {
        std::size_t             buf_size = 0;
        std::vector<impl_type*> all;   // every impl* in this tier (owned)
        std::vector<impl_type*> free;  // currently unloaned impl*s
    };

    // -----------------------------------------------------------------------
    // Shared state whose lifetime is extended by outstanding leases

    struct pool_state {
        std::vector<bucket>          buckets; // sorted ascending by buf_size
        thallium::mutex              mtx;
        thallium::condition_variable cond;
        engine                       eng;
        bulk_mode                    mode = bulk_mode::read_write;
        A                            alloc;

        // Called when the last shared_ptr<pool_state> is released.
        // By that point every custom deleter (one per outstanding lease) has
        // already run and returned its impl* to bucket.free, so deleting
        // everything in bucket.all is safe.
        ~pool_state() noexcept {
            for(auto& b : buckets)
                for(auto* p : b.all) delete p;
        }
    };

    std::shared_ptr<pool_state> m_state;

    // -----------------------------------------------------------------------
    // Helpers

    // Pure predicate (no side-effects): is there a free impl >= min_size?
    static bool has_free(const pool_state& ps, std::size_t min_size) noexcept {
        for(const auto& b : ps.buckets)
            if(b.buf_size >= min_size && !b.free.empty()) return true;
        return false;
    }

    // Pop the smallest suitable impl from the free list.
    // Must be called with m_state->mtx held and has_free returning true.
    static impl_type* pop_free(pool_state& ps, std::size_t min_size) noexcept {
        for(auto& b : ps.buckets) {
            if(b.buf_size >= min_size && !b.free.empty()) {
                impl_type* p = b.free.back();
                b.free.pop_back();
                return p;
            }
        }
        return nullptr;
    }

    // Pure lookup: smallest bucket with buf_size >= min_size, or nullptr.
    static bucket* find_bucket_for(pool_state& ps, std::size_t min_size) noexcept {
        for(auto& b : ps.buckets)
            if(b.buf_size >= min_size) return &b;
        return nullptr;
    }

    // Wrap impl* in a bulk_buffer with a "return-to-pool" custom deleter.
    bulk_buffer<A> make_lease(impl_type* p) {
        auto   state = m_state;   // capture shared_ptr → keeps pool_state alive
        auto   sz    = p->size;
        std::shared_ptr<impl_type> sp(p, [state, sz](impl_type* ptr) {
            std::unique_lock<thallium::mutex> lk(state->mtx);
            for(auto& b : state->buckets) {
                if(b.buf_size == sz) {
                    b.free.push_back(ptr);
                    break;
                }
            }
            state->cond.notify_one();
        });
        return bulk_buffer<A>{std::move(sp)};
    }

    // Build one tier and add it to m_state; throws and cleans up on failure.
    void add_bucket(const engine& e, std::size_t count, std::size_t sz,
                    bulk_mode mode, const A& alloc) {
        bucket b;
        b.buf_size = sz;
        try {
            for(std::size_t i = 0; i < count; ++i) {
                auto* p = new impl_type(e, sz, mode, alloc);
                b.all.push_back(p);
                b.free.push_back(p);
            }
        } catch(...) {
            for(auto* p : b.all) delete p;
            throw;
        }
        m_state->buckets.push_back(std::move(b));
    }

  public:

    /**
     * @brief Single-tier pool.
     *
     * @param e        Engine to register buffers with.
     * @param count    Number of buffers to pre-allocate.
     * @param buf_size Size of each buffer in bytes.
     * @param mode     Bulk access mode (read_only, write_only, or read_write).
     * @param alloc    Allocator instance.
     */
    bulk_buffer_pool(const engine& e, std::size_t count, std::size_t buf_size,
                     bulk_mode mode, A alloc = A{})
    : m_state(std::make_shared<pool_state>()) {
        m_state->eng   = e;
        m_state->mode  = mode;
        m_state->alloc = alloc;
        add_bucket(e, count, buf_size, mode, alloc);
    }

    /**
     * @brief Multi-tier pool (poolset-like interface).
     *
     * Creates @p npools tiers, each with @p nbufs buffers.  Buffer sizes are
     * @p first_size, @p first_size × @p size_multiple, …
     *
     * @param e             Engine to register buffers with.
     * @param npools        Number of size tiers.
     * @param nbufs         Number of buffers per tier.
     * @param first_size    Size (bytes) of the smallest tier.
     * @param size_multiple Multiplier applied to each successive tier.
     * @param mode          Bulk access mode.
     * @param alloc         Allocator instance.
     */
    bulk_buffer_pool(const engine& e, std::size_t npools, std::size_t nbufs,
                     std::size_t first_size, std::size_t size_multiple,
                     bulk_mode mode, A alloc = A{})
    : m_state(std::make_shared<pool_state>()) {
        m_state->eng   = e;
        m_state->mode  = mode;
        m_state->alloc = alloc;
        std::size_t sz = first_size;
        for(std::size_t t = 0; t < npools; ++t) {
            add_bucket(e, nbufs, sz, mode, alloc);
            sz *= size_multiple;
        }
    }

    ~bulk_buffer_pool() = default;

    // Move-only: the shared_ptr transfers; no duplication of state.
    bulk_buffer_pool(bulk_buffer_pool&&)            = default;
    bulk_buffer_pool& operator=(bulk_buffer_pool&&) = default;
    bulk_buffer_pool(const bulk_buffer_pool&)            = delete;
    bulk_buffer_pool& operator=(const bulk_buffer_pool&) = delete;

    /**
     * @brief Lease the smallest buffer whose size is >= @p min_size.
     *
     * If @p extend_if_needed is false (default), blocks (yielding the current
     * ULT) until a suitable buffer is available.
     *
     * If @p extend_if_needed is true and no suitable buffer is currently free,
     * a new buffer is allocated on the fly in the smallest tier whose size is
     * >= @p min_size and returned as a lease.  When the lease is later dropped
     * the buffer is recycled into that tier's free list like any other buffer.
     * Throws std::runtime_error if @p extend_if_needed is true but no tier has
     * buf_size >= @p min_size.
     *
     * @param min_size          Minimum required buffer size in bytes (0 = any).
     * @param extend_if_needed  If true, allocate a new buffer rather than blocking.
     */
    bulk_buffer<A> get(std::size_t min_size = 0, bool extend_if_needed = false) {
        std::unique_lock<thallium::mutex> lk(m_state->mtx);

        // Fast path: a free buffer is already available.
        if(has_free(*m_state, min_size))
            return make_lease(pop_free(*m_state, min_size));

        // Extend path: allocate a new impl on demand.
        if(extend_if_needed) {
            bucket* b = find_bucket_for(*m_state, min_size);
            if(!b) throw std::runtime_error(
                "bulk_buffer_pool::get: no bucket with buf_size >= min_size");
            std::unique_ptr<impl_type> guard(
                new impl_type(m_state->eng, b->buf_size, m_state->mode, m_state->alloc));
            b->all.push_back(guard.get());
            impl_type* p = guard.release();
            return make_lease(p);
        }

        // Blocking path.
        m_state->cond.wait(lk, [&]{ return has_free(*m_state, min_size); });
        return make_lease(pop_free(*m_state, min_size));
    }

    /**
     * @brief Try to lease a buffer without blocking.
     *
     * @param min_size Minimum required buffer size in bytes (0 = any).
     * @return A null bulk_buffer if no suitable buffer is immediately free.
     */
    bulk_buffer<A> try_get(std::size_t min_size = 0) {
        std::unique_lock<thallium::mutex> lk(m_state->mtx);
        impl_type* p = pop_free(*m_state, min_size);
        if(!p) return bulk_buffer<A>{};
        return make_lease(p);
    }

    /**
     * @brief The size of the largest buffer tier in bytes.
     */
    std::size_t max_buffer_size() const noexcept {
        if(!m_state || m_state->buckets.empty()) return 0;
        return m_state->buckets.back().buf_size;
    }
};

} // namespace thallium

#endif /* __THALLIUM_BULK_BUFFER_POOL_HPP */
