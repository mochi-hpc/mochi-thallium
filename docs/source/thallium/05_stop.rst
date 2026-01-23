Properly stopping a Thallium server
===================================

Stopping a server is done by calling the :code:`engine::finalize()` function.
By passing the :code:`engine` object into the closure of a lambda, as a reference,
we can make this very easy. For example:

.. literalinclude:: ../../examples/thallium/05_stop/server.cpp
       :language: cpp

In this version of the server, the server will shut down after the first RPC is received.
Note that the engine is captured by reference. Thallium engines are indeed non-copyable.
