Non-blocking RPCs
=================

Non-blocking RPCs are available in Thallium using the :code:`async()` method
of the :code:`callable_remote_procedure` objects. In this example, we take
again the sum server and show how to use the :code:`async()` function on the client.

Server
------

As a reminder, here is the server code.

.. literalinclude:: ../../examples/thallium/14_async/server.cpp
       :language: cpp

Client
------

Let's now take a look at the client code.

.. literalinclude:: ../../examples/thallium/14_async/client.cpp
       :language: cpp

Timeout
-------

There is an equivalent :code:`timed_async` function that allows one to specify a
timeout value in the form of an :code:`std::chrono::duration` object. If the RPC
times out, :code:`wait()` on the resulting :code:`async_response` object will
throw a :code:`timeout` exception.
