Serialization with context
==========================

In some situations, serializing or deserializing C++ objects require
some context, such as a factory object, some parameters, etc.
Thallium allows passing a context to its serialization mechanism
whenever needed.

Client
------

The following client example sends a "process" RPC that takes two
:code:`point` objects. By calling :code:`with_serialization_context()`
when creating the callable, we can pass any variable we want. By
default these variables will be copied into an internal context.
If a reference is needed, :code:`std::ref()` and :code:`std::cref()` can
be used.

Similarly, calling :code:`with_serialization_context` on the
response will allow the use of a context when deserializing the
response.

.. literalinclude:: ../../examples/thallium/16_context/client.cpp
       :language: cpp

Server
------

On the server side, shown bellow, note that although the RPC takes
two :code:`point` objects as argument, our lambda only takes a
:code:`tl::request`. Doing so allows us to get the argument later
on, when the context is known, by using :code:`get_input()` then
:code:`with_serialization_context()`.

Similarly, we can add :code:`with_serialization_context()` before
calling :code:`respond()` to provide a context for the serialization
of the response's data.

.. literalinclude:: ../../examples/thallium/16_context/server.cpp
       :language: cpp

Type
----

The context provided in the calls above is accessible in serialization
functions such as :code:`serialize` or :code:`load/store`, using
:code:`get_context()` on the archive parameter. This function returns
a reference to a tuple containing the provided context variables.

It is of course possible to require different types of context when
serializing and when deserializing, simply by using the :code:`load/store`.

The code bellow exemplifies accessing the context in a :code:`point` class.

.. literalinclude:: ../../examples/thallium/16_context/point.hpp
       :language: cpp

