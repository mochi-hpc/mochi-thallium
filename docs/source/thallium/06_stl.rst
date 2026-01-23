Sending and returning STL containers
====================================

So far we have passed integers as arguments to the RPC handler
and returned integers as well. Note that Thallium is able to understand
and serialize all arithmetic types, that is, all integer types
(char, long, uint64_t, etc.) and floating point types (float, double, etc.).
Provided that the type(s) they contain are serializable, Thallium is also
capable of serializing all containers of the C++14 Standard Template Library.
For instance, Thallium will be able to serialize the following type:

:code:`std::vector<std:tuple<std::pair<int,double>,std::list<int>>>`

Indeed, Thallium knows how to serialize ints and doubles, so it knows
how to serialize :code:`std::pair<int,double>` and :code:`std::list<int>`,
so it knows how to serialize :code:`std:tuple<std::pair<int,double>,std::list<int>>`,
so it knows how to serialize :code:`std::vector<std:tuple<std::pair<int,double>,std::list<int>>>`.

In order for Thallium to know how to serialize a given type,
however, you need to include the proper header in the files containing
code requiring serialization. For instance to make Thallium understand
how to serialize an :code:`std::vector`,
you need :code:`#include <thallium/serialization/stl/vector.hpp>`.
Thallium uses the Cereal serialization library, so you can also include the appropriate
header directly from the Cereal library.

The following is a revisited Hello World example in which the client sends its name as an :code:`std::string`.

Server
------

.. literalinclude:: ../../examples/thallium/06_stl/server.cpp
       :language: cpp

Client
------

.. literalinclude:: ../../examples/thallium/06_stl/client.cpp
       :language: cpp

.. note::
   We explicitly define :code:`std::string name = "Matthieu";`
   before passing it as an argument. If we were to write :code:`hello.on(server)("Matthieu");`,
   the compiler would consider :code:`"Matthieu"` as a :code:`const char*` variable, not a :code:`std::string`,
   and Thallium is not able to serialize pointers. Alternatively, :code:`hello.on(server)(std::string("Matthieu"));`
   is valid.
