POET
====

POET is a header-only C++ library for compile-time unrolling and runtime-to-compile-time
specialization: ``static_for`` (compile-time unrolled loops), ``dynamic_for`` (runtime
loops emitted as compile-time unrolled blocks), and ``dispatch``/``dispatch_set``
(runtime choice mapped to compile-time specializations). It also exposes compile-time
CPU detection: ISA, vector-width, and cache-line queries. See the guides below for the
contract of each primitive.

Project links
-------------

- `GitHub repository <https://github.com/flatironinstitute/poet>`_
- `Issue tracker <https://github.com/flatironinstitute/poet/issues>`_
- `Releases <https://github.com/flatironinstitute/poet/releases>`_
- `License (MIT) <https://github.com/flatironinstitute/poet/blob/main/LICENSE>`_
- `Single-header build <https://github.com/flatironinstitute/poet/tree/single-header>`_

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   install

.. toctree::
   :maxdepth: 2
   :caption: Guides

   guides/static_for
   guides/dynamic_for
   guides/dispatch
   guides/cpu_info
   guides/benchmarks

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/library_root
