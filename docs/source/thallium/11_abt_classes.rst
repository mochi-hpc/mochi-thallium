List of Argobots wrapper classes
================================

+--------------------+---------------------+
| Thallium class     | Argobot type        |
+====================+=====================+
| barrier            | ABT_barrier         |
+--------------------+---------------------+
| condition_variable | ABT_cond            |
+--------------------+---------------------+
| eventual           | ABT_eventual        |
+--------------------+---------------------+
| future             | ABT_future          |
+--------------------+---------------------+
| mutex              | ABT_mutex           |
+--------------------+---------------------+
| pool               | ABT_pool            |
+--------------------+---------------------+
| recursive_mutex    | ABT_mutex           |
+--------------------+---------------------+
| rwlock             | ABT_rwlock          |
+--------------------+---------------------+
| scheduler          | ABT_sched           |
+--------------------+---------------------+
| task               | ABT_task            |
+--------------------+---------------------+
| thread             | ABT_thread          |
+--------------------+---------------------+
| timer              | ABT_timer           |
+--------------------+---------------------+
| xstream            | ABT_xstream         |
+--------------------+---------------------+
| xstream_barrier    | ABT_xstream_barrier |
+--------------------+---------------------+


The following class are resource-managed (that is,
they create a valid internal Argobot handle when their
constructor is called, destroy it when their destructor
is called, they are non-copy-constructible,
non-copy-assignable, but they are move-constructible
and move-assignable): :code:`barrier`, :code:`condition_variable`,
:code:`eventual`, :code:`future`, :code:`mutex`, :code:`recursive_mutex`,
:code:`rwlock`, :code:`timer`, :code:`xstream_barrier`.

The following classes are only wrappers to Argobots resources
but do not destroy them when their destructor is called:
:code:`pool`, :code:`scheduler`, :code:`task`, :code:`thread`, :code:`xstream`.
To manage the resource inside these classes, you need to use the :code:`managed<T>`
template class (e.g. :code:`managed<thread>`). Managed resources are created
using the `create()` factory functions in their respective classes,
or the `make_thread` and `make_task` functions of the `pool` and `xstream`
classes for `managed<thread>` and `managed<task>`.

.. warning:: 
   An issue with Argobots currently makes `managed<pool>` and
   `managed<scheduler>` not behave correctly. Their internal resource will be
   freed automatically if they are default pools or default schedulers.
   If they are custom pools and schedulers they will not be freed at all.
