.. _ThalliumBulkBuffer:

Pre-allocated RDMA buffers and pools
=====================================

Registering memory for RDMA on every RPC call is expensive: each call to
:code:`engine::expose` invokes a Margo (HG/NA) memory-registration function
that can take tens of microseconds.  For high-throughput services it is more
efficient to pre-allocate and pre-register a fixed set of buffers at startup
and reuse them for every incoming transfer.  Thallium provides two classes for
this purpose:

* :code:`thallium::bulk_buffer<A>` — owns a backing allocation (via allocator
  :code:`A`) and the corresponding RDMA registration, and can be safely copied
  (all copies share the same allocation and registration via reference counting).
* :code:`thallium::bulk_buffer_pool<A>` — manages a multi-tier pool of
  :code:`bulk_buffer` objects and leases them to callers on demand.

These classes require no changes to the client side.  Only the server needs to
use them.

bulk_buffer
-----------

A :code:`bulk_buffer<A>` allocates :code:`size` bytes via allocator :code:`A`
and registers them with the engine in one step:

.. code-block:: cpp

   #include <thallium.hpp>
   namespace tl = thallium;

   tl::engine engine("tcp", THALLIUM_SERVER_MODE);

   // Allocate 1 MiB and register it for write-only RDMA.
   tl::bulk_buffer<> buf(engine, 1024*1024, tl::bulk_mode::write_only);

   void* ptr  = buf.data();   // pointer to the backing memory
   std::size_t n = buf.size();   // size in bytes

The object is copyable; all copies share the same backing memory:

.. code-block:: cpp

   tl::bulk_buffer<> copy = buf;   // both 'copy' and 'buf' point to the same memory
   assert(copy.data() == buf.data());
   assert(buf.use_count() == 2);   // reference count

The memory and RDMA registration are freed only when the last copy is destroyed.

RDMA transfers with bulk_buffer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:code:`bulk_buffer` exposes the same transfer operators as a local
:code:`thallium::bulk`:

.. code-block:: cpp

   // Pull data from a remote bulk into buf:
   remote_bulk.on(endpoint) >> buf;

   // Push data from buf to a remote bulk:
   buf >> remote_bulk.on(endpoint);

   // Async variants:
   tl::async_bulk_op op = buf.pull_from(remote_bulk.on(endpoint));
   std::size_t bytes_transferred = op.wait();

A typical server-side handler that receives data from a client:

.. code-block:: cpp

   tl::bulk_buffer<> server_buf(engine, 1024*1024, tl::bulk_mode::write_only);

   engine.define("recv", [&server_buf](const tl::request& req, tl::bulk& remote) {
       remote.on(req.get_endpoint()) >> server_buf;
       // process bytes at server_buf.data() ...
       req.respond(0);
   });

bulk_buffer_pool
----------------

Allocating one :code:`bulk_buffer` per request still avoids repeated
registration (the buffers are pre-allocated), but forces a blocking wait when
all buffers are in use.  :code:`bulk_buffer_pool<A>` manages a fixed set of
buffers and lets the server borrow them as leases.

Single-tier pool
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 4 buffers of 1 MiB each, registered write-only.
   tl::bulk_buffer_pool<> pool(engine, /*count=*/4, /*size=*/1024*1024,
                                tl::bulk_mode::write_only);

   engine.define("recv", [&pool](const tl::request& req, tl::bulk& remote) {
       tl::bulk_buffer<> buf = pool.get();   // blocks until a buffer is free
       remote.on(req.get_endpoint()) >> buf;
       // process buf.data() ...
       req.respond(0);
   }); // buf destroyed here → automatically returned to the pool

:code:`pool.get()` blocks (yielding the current ULT) until a buffer is
available.  Use :code:`pool.try_get()` for a non-blocking attempt that returns
a null :code:`bulk_buffer` (i.e., :code:`buf.is_null() == true`) if all slots
are in use.

On-demand growth with ``extend_if_needed``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Passing :code:`true` as the second argument to :code:`get()` causes the pool to
allocate a new buffer on the fly when no free buffer is available, rather than
blocking:

.. code-block:: cpp

   tl::bulk_buffer<> buf = pool.get(min_size, /*extend_if_needed=*/true);

When :code:`extend_if_needed` is :code:`true`:

* If a free buffer already exists it is returned immediately (no allocation).
* Otherwise a new buffer is allocated in the smallest tier whose size is
  >= :code:`min_size` and returned as a lease.  When the lease drops the buffer
  is recycled into that tier's free list exactly like a pre-allocated buffer.
* If no existing tier has a large-enough buffer size, a new tier of exactly
  :code:`min_size` bytes is created on the fly and appended to the pool.
  Subsequent calls that fit within this new tier will reuse it.

This allows creating a pool with zero pre-allocated buffers (:code:`count=0` /
:code:`nbufs=0`) and letting it grow on demand:

.. code-block:: cpp

   // 3 tiers, no pre-allocated buffers — grow as needed.
   tl::bulk_buffer_pool<> pool(engine, /*npools=*/3, /*nbufs=*/0,
                                /*first_size=*/64*1024, /*size_multiple=*/4,
                                tl::bulk_mode::write_only);

   engine.define("recv", [&pool](const tl::request& req, tl::bulk& remote) {
       // Picks the smallest tier >= remote.size(); allocates if necessary.
       tl::bulk_buffer<> buf = pool.get(remote.size(), true);
       remote.on(req.get_endpoint()) >> buf;
       req.respond(0);
   });

Multi-tier pool
~~~~~~~~~~~~~~~

A multi-tier pool (analogous to a *poolset*) holds several size classes so
that small transfers are not forced to use an oversized buffer:

.. code-block:: cpp

   // 3 tiers: 64 KiB, 256 KiB, 1 MiB; 4 buffers per tier.
   tl::bulk_buffer_pool<> pool(engine,
       /*npools=*/3, /*nbufs=*/4,
       /*first_size=*/64*1024, /*size_multiple=*/4,
       tl::bulk_mode::write_only);

   // max_buffer_size() returns the largest tier (1 MiB here).
   assert(pool.max_buffer_size() == 1024*1024);

   engine.define("recv", [&pool](const tl::request& req, tl::bulk& remote) {
       // Borrow the smallest available buffer that fits the incoming data.
       tl::bulk_buffer<> buf = pool.get(remote.size());
       remote.on(req.get_endpoint()) >> buf;
       req.respond(0);
   });

The :code:`get(min_size)` and :code:`try_get(min_size)` overloads select the
smallest tier whose buffer size is at least :code:`min_size` bytes.

Pool and lease lifetimes
~~~~~~~~~~~~~~~~~~~~~~~~~

The pool's internal state is managed through a :code:`shared_ptr` and is
therefore kept alive as long as at least one outstanding lease exists, even
if the :code:`bulk_buffer_pool` object itself has been destroyed.  This makes
it safe to capture the pool by reference in a long-running RPC handler and
then destroy the pool object before all outstanding RPCs have completed.

.. code-block:: cpp

   tl::bulk_buffer<> held;
   {
       tl::bulk_buffer_pool<> pool(engine, 1, 256, tl::bulk_mode::write_only);
       held = pool.get();
   } // pool destroyed; pool_state kept alive by 'held'

   assert(!held.is_null());   // still valid
   held = tl::bulk_buffer<>();  // last reference dropped → pool_state freed

Custom allocators
-----------------

Both :code:`bulk_buffer<A>` and :code:`bulk_buffer_pool<A>` accept an
allocator template parameter (defaulting to :code:`std::allocator<char>`).
Any allocator whose :code:`value_type` is :code:`char` can be used.

.. code-block:: cpp

   // Use a custom allocator (e.g., a huge-page allocator):
   my_hugepage_allocator<char> alloc;
   tl::bulk_buffer<my_hugepage_allocator<char>> buf(engine, 2*1024*1024,
                                                    tl::bulk_mode::read_write,
                                                    alloc);

API summary
-----------

.. code-block:: cpp

   // ---- bulk_buffer<A> ----

   // Construct a null buffer (is_null() == true):
   bulk_buffer();

   // Allocate size bytes and register for RDMA:
   bulk_buffer(engine& e, std::size_t size, bulk_mode mode, A alloc = A{});

   bool          is_null()   const noexcept;
   std::size_t   size()      const noexcept;
   void*         data()      const noexcept;
   std::uint32_t use_count() const noexcept;

   remote_bulk   on(const endpoint& ep)          const noexcept;
   std::size_t   operator>>(const remote_bulk&)  const;
   std::size_t   operator<<(const remote_bulk&)  const;
   async_bulk_op push_to(const remote_bulk&)     const;
   async_bulk_op pull_from(const remote_bulk&)   const;

   // Free function — enables "remote_bulk >> buf" notation:
   template <typename A>
   std::size_t operator>>(const remote_bulk& rb, const bulk_buffer<A>& buf);

   // ---- bulk_buffer_pool<A> ----

   // Single-tier:
   bulk_buffer_pool(engine& e, std::size_t count, std::size_t buf_size,
                    bulk_mode mode, A alloc = A{});

   // Multi-tier (npools tiers, nbufs per tier,
   // sizes: first_size, first_size*size_multiple, …):
   bulk_buffer_pool(engine& e, std::size_t npools, std::size_t nbufs,
                    std::size_t first_size, std::size_t size_multiple,
                    bulk_mode mode, A alloc = A{});

   // Blocks until a buffer >= min_size is free; or allocates one if extend_if_needed.
   bulk_buffer<A> get(std::size_t min_size = 0, bool extend_if_needed = false);
   bulk_buffer<A> try_get(std::size_t min_size = 0);  // returns null if none free
   std::size_t    max_buffer_size() const noexcept;   // largest tier size
