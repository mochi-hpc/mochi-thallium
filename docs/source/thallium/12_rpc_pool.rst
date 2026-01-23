Using Argobots pools with Thallium RPCs
=======================================

Thallium allows RPC handlers to be associated with a particular
Argobots pool, so that any incoming request for that RPC gets
dispatched to the specified pool.

Server
------

The following code exemplifies using a custom pool in a server.

.. literalinclude:: ../../examples/thallium/12_rpc_pool/server.cpp
       :language: cpp

We are explicitly calling :code:`wait_for_finalize()`
(which is normally called in the engine's destructor)
before joining the execution streams because we don't
want the primary ES to be blocking on the :code:`join()` calls.

We are also using a :code:`tl::abt` object to initialize
Argobots because this prevents the engine from taking
ownership of the Argobots environment and destroy it
on the :code:`wait_for_finalize()` call.

.. important::
   This feature requires to provide a non-zero provider
   id (passed to the define call) when defining the RPC
   (here 1). Hence you also need to use provider handles
   on clients, even if you do not define a provider class.

Client
------

Here is the corresponding client.

.. literalinclude:: ../../examples/thallium/12_rpc_pool/client.cpp
       :language: cpp
