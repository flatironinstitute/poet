Static Loops
============

``poet::static_for`` expands an integer range at compile time.

Basic form
----------

.. code-block:: cpp

   poet::static_for<0, 4>([](auto I) {
       use(I);
   });

The callable may also be a template call operator. The explicit lambda
template parameter list shown here needs ``-std=c++20`` or later; the rest of
``static_for`` is C++17.

.. code-block:: cpp

   poet::static_for<4>([]<auto I>() {
       use_compile_time_index<I>();
   });

Step and direction
------------------

.. code-block:: cpp

   poet::static_for<0, 10, 2>([](auto I) {
       use(I);// 0, 2, 4, 6, 8
   });

   poet::static_for<9, 4, -1>([](auto I) {
       use(I);// 9, 8, 7, 6, 5
   });

Block size
----------

The default block size covers the full range. Pass the fourth template parameter
when a large body benefits from smaller outlined blocks:

.. code-block:: cpp

   poet::static_for<0, 64, 1, 8>([](auto I) {
       heavy_work(I);
   });

Use this only when profiling shows register-pressure or compile-time issues.

Guarantees
----------

- Every index in ``[Begin, End)`` is visited exactly once, in order, at compile
  time; the body is emitted, not run in a runtime loop.
  Verified by: ``exact_unroll`` (the ``static_for8`` case, ``tests/exact_unroll_check.cpp``).
- An empty range (``Begin == End``) calls the body 0 times.
  Verified by: ``static_for empty range never calls the body`` (``tests/static_for_tests.cpp``).

When not to use
----------------

Avoid ``static_for`` for large ranges: compile time and code size grow linearly
with ``End - Begin``. Use ``dynamic_for`` when the range is large or its size is
not known at compile time.

Runnable example
----------------

- Full source: `examples/static_for.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/static_for.cpp>`_
- |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/static_for.html>`_

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
