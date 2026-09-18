Dispatch
========

``poet::dispatch`` maps runtime integers to compile-time specializations.

Single parameter
----------------

.. code-block:: cpp

   struct Kernel {
       template<int N>
       int operator()(int x) const {
           return N + x;
       }
   };

   int result = poet::dispatch(
       Kernel{},
       poet::dispatch_param<poet::inclusive_range<0, 4>>{choice},
       10);

``poet::inclusive_range<Start, End>`` is inclusive on both ends.

Multiple parameters
-------------------

Pass multiple ``dispatch_param`` objects to dispatch over a cartesian product:

.. code-block:: cpp

   struct Kernel2D {
       template<int R, int C>
       int operator()(int seed) const { return R * 100 + C * 10 + seed; }
   };

   auto result = poet::dispatch(
       Kernel2D{},
       poet::dispatch_param<poet::inclusive_range<1, 4>>{rows},
       poet::dispatch_param<poet::inclusive_range<1, 4>>{cols},
       7);

The tuple form is equivalent:

.. code-block:: cpp

   auto params = std::make_tuple(
       poet::dispatch_param<poet::inclusive_range<1, 4>>{rows},
       poet::dispatch_param<poet::inclusive_range<1, 4>>{cols});

   poet::dispatch(Kernel2D{}, params, 7);

Sparse combinations
-------------------

Use ``dispatch_set`` when only specific tuples are valid:

.. code-block:: cpp

   struct MatMul {
       template<int R, int C>
       int operator()(int a, int b) const { return R * a + C * b; }
   };

   using Shapes = poet::dispatch_set<int,
       poet::values<2, 2>,
       poet::values<4, 4>,
       poet::values<2, 4>>;

   int result = poet::dispatch(MatMul{}, Shapes{rows, cols}, 3, 5);

Error handling
--------------

The default behavior on a miss is:

- ``void`` return: do nothing
- non-``void`` return: return ``R{}``

Use ``poet::throw_on_no_match`` when a miss should fail:

.. code-block:: cpp

   auto result = poet::dispatch(
       poet::throw_on_no_match,
       Kernel{},
       poet::dispatch_param<poet::inclusive_range<0, 4>>{choice},
       10);

The same tag works with ``dispatch_set``.

Guarantees
----------

- Exactly one specialization is called on a match.
  Verified by: ``dispatch routes to the matching template instantiation``
  (``tests/dispatch_tests.cpp``).
- On a miss: ``void`` does nothing, non-``void`` returns a value-initialized ``R``.
  Verified by: ``dispatch returns default values when no match exists`` and
  ``dispatch handles void return type explicitly`` (``tests/dispatch_tests.cpp``).
- ``throw_on_no_match`` throws ``poet::no_match_error``, which derives from
  ``std::runtime_error``.
  Verified by: ``dispatch with throw_on_no_match variadic form``/``tuple form``
  and ``dispatch_set throws when requested and no match`` (``tests/dispatch_tests.cpp``).
- The index computation never uses integer division.
  Verified by: ``exact_unroll`` (``dispatch_contig16``, ``dispatch_sparse5``,
  ``dispatch_2d``, ``dispatch_strided8``, ``tests/exact_unroll_check.cpp``).
- ``dispatch`` is not usable in a constant expression.
- ``dispatch_set`` accepts both the value form
  (``functor(std::integral_constant<V, Value>{}..., args...)``) and the template
  form (``functor.template operator()<Value...>(args...)``); the value form is
  preferred when both are viable.

When not to use
----------------

Call the template form directly when the value is known at compile time;
``dispatch`` only pays off when the choice is a runtime value. Avoid it when
the set of combinations is very large: table size grows with the product of
the ranges (or the tuple count, for ``dispatch_set``).

Runnable examples
-----------------

- `examples/dispatch.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/dispatch.cpp>`_:
  single and multi-parameter dispatch.
  |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/dispatch.html>`_
- `examples/dispatch_set.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/dispatch_set.cpp>`_:
  sparse combinations and ``throw_on_no_match``.
  |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/dispatch_set.html>`_
- `examples/polynomial.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/polynomial.cpp>`_:
  Horner's method specialized on a runtime-chosen degree, combined
  with ``static_for`` to unroll the recurrence.
  |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/polynomial.html>`_

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
