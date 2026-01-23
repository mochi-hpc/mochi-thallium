Thallium (Core C++ runtime)
===========================

This section will walk you through a series of tutorials on how to use
Thallium. We highly recommend to read at least up to the tutorial
on providers, which will give you a good picture of how to make a truely
object-oriented Mochi service with Thallium.

Thallium also provide a complete object-oriented wrapper for Argobots.
One important thing to keep in mind is that these wrappers should be used
in place of the C++ threading library.

.. important::
   One of the most frequent source
   of bug we encounter is developers mistakenly using :code:`std::mutex`
   or :code:`std::thread` intead of their Thallium counterparts
   :code:`thallium::mutex` and :code:`thallium::thread`.

.. toctree::
   :maxdepth: 1

   thallium/01_init.rst
   thallium/02_hello.rst
   thallium/03_args.rst
   thallium/04_lambdas.rst
   thallium/05_stop.rst
   thallium/06_stl.rst
   thallium/07_serialization.rst
   thallium/08_rdma.rst
   thallium/14_async.rst
   thallium/09_providers.rst
   thallium/15_finalize.rst
   thallium/10_abt_intro.rst
   thallium/11_abt_classes.rst
   thallium/12_rpc_pool.rst
   thallium/13_abt_custom.rst
   thallium/16_context.rst
   thallium/17_logging.rst
   thallium/18_timed_cb.rst
   thallium/19_config.rst
   thallium/cpp_api.rst
