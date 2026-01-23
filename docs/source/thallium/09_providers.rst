Working in terms of providers
=============================

It is often desirable for RPC to target a specific instance
of a class on the server side. Classes that can accept RPC
requests are called *providers*. A provider object is characterized
by a provider id (of type :code:`uint16_t`). You will need to make
sure that no two providers use the same provider id.

Server
------

The following code sample illustrates a custom
provider class, :code:`my_sum_provider`, which exposes a number
of its methods as RPC.

.. literalinclude:: ../../examples/thallium/09_providers/server.cpp
       :language: cpp

This code defines the :code:`my_sum_provider` class, and creates
an instance of this class (passing the :code:`engine` as parameter
and a provider id). The :code:`my_sum_provider` class inherits
from :code:`thallium::provider<my_sum_provider>` to indicate that
this is a provider.

The RPC methods are exposed in the class constructor using the
:code:`define` method of the base provider class. Note that
multiple definitions of members are possible and exemplified here.

- "prod" is defined the same way as we previously defined RPCs using the engine,
  with a function that returns :code:`void` and takes a :code:`const thallium::request&` as first parameter.
- "sum" is defined without the :code:`const thallium::request&` parameter. Since it returns an :code:`int`,
  Thallium will assume that this is what needs to be sent back to the client. It will therefore respond
  to the client with this return value.
- "hello" does not have a :code:`const thallium::request&` parameter either, and returns :code:`void`.
  Thallium will implicitly call :code:`.disable_response()` on this RPC to indicate that it does 
  not send a response back to the client.
- "print" does not have a :code:`const thallium::request&` parameter, and returns an :code:`int`.
  By default Thallium would consider that we want this return value to be sent to the client.
  To prevents this, we add the :code:`thallium::ignore_return_value()` argument, which indicates
  Thallium that the function should be treated as if it returned void.

Client
------

Let's now take a look at the client code.

.. literalinclude:: ../../examples/thallium/09_providers/client.cpp
       :language: cpp

This client takes a provider id in addition to the server's address.
It uses it to define a :code:`thallium::provider_handle` object encapsulating
the server address and the provider id. This provider handle is then used in
place of the usual :code:`thallium::endpoint` to send RPCs to a specific
instance of provider.

.. important::
   We have called :code:`disable_response()` on the "hello" RPC
   here because there is no way for Thallium to infer here that this RPC
   does not send a response.
