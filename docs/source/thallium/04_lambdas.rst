Using lambdas and objects to define RPC handlers
================================================

So far we have seen how to define an RPC handler using a function.
This method is not necessarily the best, since it forces the use of
global variables to access anything outside the function.

Fortunately, Thallium can use lambdas as RPC handlers, and even
any object that has parenthesis operator overloaded.
Here is how to rewrite the sum server using a lambda.

.. literalinclude:: ../../examples/thallium/04_lambdas/server.cpp
       :language: cpp

The big advantage of lambdas is their ability to capture local variables,
which prevents the use of global variables to pass user-provided data into
RPC handlers. This will become handy in the next tutorial...
