Sending and returning custom classes
====================================

We have seen previously that all STL containers can be
serialized and deserialized by Thallium. Thallium can also
serialize and deserialize user-defined classes, with a little help.
This help comes in the form of a :code:`serialize` function template
put inside the class.

3D point example
----------------

Here is an example with a point class representing a 3d point.

.. literalinclude:: ../../examples/thallium/07_serialization/point.hpp
       :language: cpp

You will also need the class to be default-constructible.

The template parameter (A) and the function's parameter (ar)
represent an archive, from which one can serialize/deserialize data.
Note that you don't need to provide two separate functions:
Thallium is smart enough to use the same :code:`serialize` function for
both reading and writing, depending on the context.

The :code:`serialize` function calls the operator :code:`&` of
the archive to either write or read the class' data members.
Provided that those data members are basic types, user-defined
types with a :code:`serialize` method, or STL containers of a
serializable type, Thallium will know how to serialize the class.

.. important::
   Thallium needs classes used as RPC argument/response to be
   default-constructible (i.e. provide a constructor that takes
   no argument).

Asymmetric serialization
------------------------

In some cases it may not be possible for the serialize
function to present a symmetric behavior for reading and for
writing. For instance this may happen if the deserialization
function needs to allocate a pointer. In these cases, one can,
instead of a :code:`serialize` method, provide a :code:`save` 
method and a :code:`load` method. :code:`save` will be used
for serialization, :code:`load` will be used for deserialization.

Reading/writing raw data
------------------------

It may also be convenient to read or write raw data from/to
the archive. For this, note that archive objects used for
serialization provide a :code:`write` method:

.. code-block:: cpp

   template<typename T> write(T* const t, std::size_t count=1)

and archive objects used for deserialization provide a :code:`read` method:

.. code-block:: cpp

   template<typename T> read(T* t, std::size_t count=1)

The :code:`write` method should be used only in a :code:`save` template
method, while the :code:`read` method should be used only in a :code:`load` template method.

Non-member serialization functions
----------------------------------

In some contexts it may not be possible to add member functions to a class.
In those cases, you can add :code:`serialize` or :code:`load`/:code:`save` functions
outside of the class, as follows:

.. code-block:: cpp

   template<typename A>
   void serialize(A& ar, MyType& x) {
     ...
   }

or

.. code-block:: cpp

   template<typename A>
   void save(A& ar, const MyType& x) {
     ...
   }

   template<typename A>
   void load(A& ar, MyType& x) {
     ...
   }
