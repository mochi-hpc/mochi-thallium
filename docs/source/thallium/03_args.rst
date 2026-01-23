Sending arguments, returning values
===================================

In this example we will define a "sum" RPC that will take two integers
and return their sum.

Server
------

Here is the server code.

.. literalinclude:: ../../examples/thallium/03_args/server.cpp
       :language: cpp

Notice that our :code:`sum` function now takes two integers in addition
to the const reference to a :code:`thallium::request`. You can also see
that this request object is used to send a response back to the client.
Because the server now sends something back to the client, we do not call
:code:`ignore_response()` when defining the RPC.

Client
------

Let's now take a look at the client code.

.. literalinclude:: ../../examples/thallium/03_args/client.cpp
       :language: cpp

The client calls the remote procedure with two integers and gets an integer back.
This way of passing parameters and returning a value hides many implementation
details that are handled with a lot of template metaprogramming.
Effectively, what happens is the following.
When passing the :code:`sum` function to :code:`engine::define`, the compiler
deduces from its signature that clients will send two integers.
Thus it creates the code necessary to deserialize two integers
before calling the function.

On the client side, calling :code:`sum.on(server)(42,63)` makes the compiler
realize that the client wants to serialize two integers and send them
along with the RPC. It therefore also generates the code for that.
The same happens when calling :code:`req.respond(...)` in the server,
the compiler generates the code necessary to serialize whatever object has been passed.

Back on the client side, :code:`sum.on(server)(42,63)` does not actually return an integer.
It returns an instance of :code:`thallium::packed_response`, which can be cast into any type,
here an integer. Asking the :code:`packed_response` to be cast into an integer also instructs
the compiler to generate the right deserialization code.

.. warning::
   A common miskate consists of changing the arguments accepted by an RPC handler
   but forgetting to update the calls to that RPC on clients. This can lead to data
   corruptions or crashes. By default, Thallium has no way to check that the types passed
   by the client to the RPC call are the ones expected by the server. To debug such
   problems, you can compile your code with :code:`-DTHALLIUM_DEBUG_RPC_TYPES` or
   link it (with cmake) against the :code:`thallium_check_types` target. This
   will add the name of the type as a payload to the RPC (hence increasing the size of
   these payloads) and will check that the type matches upon deserialization.

.. warning::
   Another common mistake is to use integers of different size on client and server.
   For example :code:`sum.on(server)(42,63);` on the client side will serialize two
   int values, because int is the default for integer litterals. If the corresponding
   RPC handler on the server side had been
   :code:`void sum(const tl::request& req, int64_t x, int64_t y)`,
   the call would have led to data corruptions and potential crash. One way to ensure
   that the right types are used is to explicitely cast the litterals:
   :code:`sum.on(server)(static_cast<int64_t>(42), static_cast<int64_t>(63));`,
   or assign them to variables of a known type.

Timeout
-------

It can sometime be useful for an operation to be given a certain amount of time before
timing out. This can be done using the :code:`callable_remote_procedure::timed()`
function. This function behaves like the :code:`operator()` but takes a first parameter
of type :code:`std::chrono::duration` representing an amount of time after which
the call will throw a :code:`thallium::timeout` exception. For instance in the above
client code, :code:`int ret = sum.on(server)(42,63);` would become
:code:`int ret = sum.on(server).timed(std::chrono::milliseconds(5), 42 ,63);` to
allow for a 5ms timeout.
