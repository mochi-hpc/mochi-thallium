Finalization callbacks
======================

In the previous tutorial, we have seen how to implement a provider
class using Thallium. It may be convenient for a provider to know
when the engine is being finalized, so as to carry out any
necessary cleanup.


The following code sample illustrates another version of our custom
provider class, :code:`my_sum_provider`, which pushes a finalization callback
that will be executed when the engine is finalized.

.. literalinclude:: ../../examples/thallium/15_finalize/server.cpp
       :language: cpp

:code:`push_finalize_callback` installs a callback that will be called
when the engine is finalized, after Mercury resources have been destroyed.
Hence, such a callback cannot make RPC calls.

Another method, :code:`push_prefinalize_callback`, can be used to install
a callback to invoke when the engine is being finalized but before any
cleanup of the Mercury resources.
