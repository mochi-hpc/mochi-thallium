Using timed callbacks
=====================

Thallium's counterpart of Margo's timers is the concept of *timed callbacks*
and the :code:`timed_callback` class, illustrated bellow.

.. literalinclude:: ../../examples/thallium/18_timed_cb/server.cpp
       :language: cpp

.. important::
   Care should be taken that :code:`timed_callback` objects do not live
   beyond the engine's finalization.
