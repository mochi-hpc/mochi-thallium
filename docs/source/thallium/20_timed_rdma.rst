.. _ThalliumTimedBulk:

Timed RDMA transfers
====================

This tutorial extends :ref:`ThalliumBulk` by showing how to attach a deadline
to bulk RDMA operations. Three new Margo functions underlie this feature:

* :code:`margo_bulk_transfer_timed` — synchronous transfer with a millisecond deadline
* :code:`margo_bulk_itransfer_timed` — asynchronous transfer with a millisecond deadline

Both are exposed through the :code:`timed()` method on :code:`thallium::remote_bulk`.

Synchronous timed transfers
---------------------------

Calling :code:`.timed(deadline)` on a :code:`remote_bulk` object returns a
:code:`timed_remote_bulk` proxy that behaves exactly like :code:`remote_bulk`,
but routes all transfers through the timed Margo functions.

Server
~~~~~~

.. literalinclude:: ../../examples/thallium/22_timed_rdma/server.cpp
       :language: cpp

The key line is:

.. code-block:: cpp

   b.on(ep).timed(std::chrono::seconds(5)) >> local;

:code:`timed()` accepts either a :code:`double` (milliseconds) or any
:code:`std::chrono::duration`:

.. code-block:: cpp

   b.on(ep).timed(5000.0)                    >> local;  // 5 s as double ms
   b.on(ep).timed(std::chrono::seconds(5))   >> local;  // std::chrono overload
   b.on(ep).timed(std::chrono::milliseconds(500)) >> local;

If the deadline expires before the transfer completes, :code:`>>` (or :code:`<<`)
throws :code:`tl::timeout`. Any other Margo error throws :code:`tl::margo_exception`.

Client
~~~~~~

.. literalinclude:: ../../examples/thallium/22_timed_rdma/client.cpp
       :language: cpp

The client side is identical to a regular RDMA client — only the server side
decides whether to use a timed transfer.

Asynchronous timed transfers
-----------------------------

:code:`timed_remote_bulk` also exposes :code:`pull_to` and :code:`push_from`,
which start the transfer without blocking and return an :code:`async_bulk_op`.
Calling :code:`wait()` on the returned object blocks until the transfer
completes or the deadline expires.

.. code-block:: cpp

   tl::async_bulk_op op = b.on(ep).timed(std::chrono::seconds(5)).pull_to(local);
   // ... overlap other computation here ...
   try {
       std::size_t n = op.wait();  // blocks; throws tl::timeout if deadline expired
   } catch(const tl::timeout&) {
       // handle timeout
   }

Similarly for :code:`push_from`:

.. code-block:: cpp

   tl::async_bulk_op op = b.on(ep).timed(5000.0).push_from(local);
   std::size_t n = op.wait();

.. note::

   A zero deadline (:code:`timed(0.0)`) is equivalent to calling the
   non-timed variants (:code:`operator>>` / :code:`operator<<` / :code:`pull_to`
   / :code:`push_from`) directly, since the underlying
   :code:`margo_bulk_transfer` and :code:`margo_bulk_itransfer` functions are
   themselves thin wrappers around their timed counterparts with
   :code:`timeout_ms = 0`.

API summary
-----------

.. code-block:: cpp

   // -- on remote_bulk --

   // Returns a timed_remote_bulk proxy (double overload)
   timed_remote_bulk remote_bulk::timed(double timeout_ms) const noexcept;

   // Returns a timed_remote_bulk proxy (std::chrono::duration overload)
   template<typename Rep, typename Period>
   timed_remote_bulk remote_bulk::timed(
       const std::chrono::duration<Rep,Period>& d) const noexcept;

   // -- on timed_remote_bulk --

   // Synchronous pull (remote → local): throws tl::timeout on expiry
   std::size_t timed_remote_bulk::operator>>(const bulk_segment& dest) const;

   // Synchronous push (local → remote): throws tl::timeout on expiry
   std::size_t timed_remote_bulk::operator<<(const bulk_segment& src) const;

   // Asynchronous pull: wait() throws tl::timeout on expiry
   async_bulk_op timed_remote_bulk::pull_to(const bulk_segment& dest) const;

   // Asynchronous push: wait() throws tl::timeout on expiry
   async_bulk_op timed_remote_bulk::push_from(const bulk_segment& src) const;
