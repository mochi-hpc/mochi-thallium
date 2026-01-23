Simple Hello World RPC
======================

We will now define a simple RPC handler that prints "Hello World".

Server
------

Let's take again the server code from the previous section and improve it.

.. literalinclude:: ../../examples/thallium/02_hello/server.cpp
       :language: cpp

The :code:`engine::define` method is used to define an RPC.
The first argument is the name of the RPC (a string), the second is a function.
This function should take a const reference to a :code:`thallium::request` as argument.
We will see in a future example what this request object is used for.
The :code:`disable_response()` method is called to indicate that the RPC is 
not going to send any response back to the client.

Client
------

Let's now take a look at the client code.

.. literalinclude:: ../../examples/thallium/02_hello/client.cpp
       :language: cpp

The client does not declare the :code:`hello` function, since its code is on the server.
Instead, it calls :code:`engine::define` with only the name of the RPC, indicating that
there exists on the server a RPC that goes by this name. Again we call 
:code:`disable_response()` to indicate that this RPC does not send a response back.
We then use the engine to perform an address lookup. This call returns an :code:`endpoint`
representing the server.

Finally we can call the :code:`hello` RPC by associating it with an :code:`endpoint`.
:code:`hello.on(server)` actually returns an instance of a class :code:`callable_remote_procedure`
which has its parenthesis operator overloaded to make it usable like a function.
