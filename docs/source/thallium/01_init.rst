Initializing Thallium
=====================

Thallium's main class is the engine. It is used to initialize the underlying libraries
(Margo, Mercury, and Argobots), to define RPCs, expose segments of memory for RDMA,
and lookup addresses.

Server
------

Here is a simple example of server.

.. literalinclude:: ../../examples/thallium/01_init/server.cpp
       :language: cpp

You can compile this program with the following command (using GCC):

.. code-block:: console

   g++ -std=c++14 -o myServer myServer.cpp -lmargo -lmercury -labt

The first argument of the constructor is the server's protocol (tcp).
You can refer to the
`Mercury documentation <http://mercury-hpc.github.io/documentation/>`_
to see a list of available protocols. The second argument,
:code:`THALLIUM_SERVER_MODE`, indicates that this engine is a server.
When running this program, it will print the server's address, then block 
on the destructor of myEngine. Server engines are indeed supposed to wait for 
incoming RPCs. We will see in the next tutorial how to properly shut it down.

Client
------

The following code initialize the engine as a client:

.. literalinclude:: ../../examples/thallium/01_init/client.cpp
       :language: cpp

You can compile this program with the following command (using GCC):

.. code-block:: console

   g++ -std=c++14 -o myServer myServer.cpp -lmargo -lmercury -labt

Contrary to the server, this program will exit normally.
Client engine are not supposed to wait for anything.
We use :code:`THALLIUM_CLIENT_MODE` to specify that the engine is a client.
We can simply provide the protocol, here "tcp", since a client is not receiving on any address.

.. important::
   A thallium engine initialized as a server (i.e. expected to receive RPCs) can also be used
   as a client (i.e. expected to send RPCs). There is no need to initialize multiple engines
   in the same process, and it is often a bad idea to do so as their respective
   progress loops will compete for network resources in an unpredictable manner.
