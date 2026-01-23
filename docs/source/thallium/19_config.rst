Margo JSON configuration in Thallium
====================================

Just like Margo can take a JSON configuration describing and Argobots
and a Mercury environment, a Thallium engine can be initialized with the
same kind of JSON configuration, as shown in the code bellow.

.. literalinclude:: ../../examples/thallium/19_config/server.cpp
       :language: cpp

Once initialized this way, one can access the pools and execution streams
defined in the configurations via the :code:`engine::pools()` and
:code:`engine::xstreams()` methods, respectively. These methods
return a *proxy* objects that allows to access pools and xstreams by
name or index. The returned objects are also proxies, allowing to access the
name and index of the pool/xstream (:code:`tl::pool` and :code:`tl::xstream`
objects don't naturally have such attributes).

These functionalities can be useful to externalize the Argobots and
Mercury setup into a configuration file instead of hard-coding
the creation and management of pools and execution streams.
