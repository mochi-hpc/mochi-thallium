.. _ThalliumBulk:

Transferring data over RDMA
===========================

In this tutorial, we will learn how to transfer data over RDMA.
The class at the core of this tutorial is :code:`thallium::bulk`.
This object represents a series of segments of memory within the
current process or in a remote process, that is exposed for remote
memory accesses.

Client
------

Here is an example of a client sending a "do_rdma" RPC with a bulk object as argument.

.. literalinclude:: ../../examples/thallium/08_rdma/client.cpp
       :language: cpp

In this client, we define a buffer with the content "Matthieu"
(because it's a string, there is actually a null-terminating character).
We then define segments as a vector of pairs of :code:`void*` and :code:`std::size_t`.
Each segment (here only one) is characterized by its starting address in local
memory and its size. We call :code:`engine::expose` to expose the buffer and
get a :code:`bulk` instance from it. We specify :code:`tl::bulk_mode::read_only`
to indicate that the memory will only be read by other processes
(alternatives are :code:`tl::bulk_mode::read_write` and :code:`tl::bulk_mode::write_only`).
Finally we send an RPC to the server, passing the bulk object as an argument.

Server
------

Here is the server code now:

.. literalinclude:: ../../examples/thallium/08_rdma/server.cpp
       :language: cpp

In the RPC handler, we get the client's :code:`endpoint` using
:code:`req.get_endpoint()`. We then create a buffer of size 6.
We initialize :code:`segments` and expose the buffer to get a :code:`bulk`
object from it. The call to the :code:`>>` operator pulls data from
the remote :code:`bulk` object :code:`b` and the local :code:`bulk` object.
Since the local :code:`bulk` is smaller (6 bytes) than the remote one (9 bytes),
only 6 bytes are pulled. Hence the loop will print *Matthi*.
It is worth noting that an :code:`endpoint` is needed for Thallium to know
in which process to find the memory we are pulling. That's what :code:`bulk::on(endpoint)`
does.

Understanding local and remote bulk objects
-------------------------------------------

A :code:`bulk` object created using :code:`engine::expose` is local.
When such a :code:`bulk` object is sent to another process, it becomes remote.
Operations can only be done between a local :code:`bulk` object and a remote :code:`bulk`
object resolved with an endpoint, e.g.,

.. code-block:: cpp

   myRemoteBulk.on(myRemoteProcess) >> myLocalBulk;

or

.. code-block:: cpp

   myLocalBulk >> myRemoteBulk.on(myRemoteProcess);

The :code:`<<` operator is, of course, also available.

Transferring subsections of bulk objects
----------------------------------------

It is possible to select part of a bulk object to be transferred.
This is done as follows, for example.

.. code-block:: cpp

   myRemoteBulk(3,45).on(myRemoteProcess) >> myLocalBulk(13,45);

Here we are pulling 45 bytes of data from the remote :code:`bulk`
starting at offset 3 into the local :code:`bulk` starting at its
offset 13. We have specified 45 as the number of bytes to be
transferred. If the sizes had been different, the smallest
one would have been picked.

.. important::

   Some people expect a streaming semantic from the :code:`>>` and
   :code:`<<` operators, i.e. as if it behaved like :code:`ostream << ...`.
   In Thallium, these operators DO NOT have a streaming semantic, i.e.
   an offset pointer is NOT updated between operations. They simply
   indicate the direction of the flow of data.
