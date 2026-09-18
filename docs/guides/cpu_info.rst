CPU Info
========

``poet::detected_isa``, ``poet::available_registers``, ``poet::cache_line``, and
the related helpers query the compiler's target predefines at compile time.

.. code-block:: cpp

   constexpr auto isa = poet::detected_isa();
   constexpr auto regs = poet::available_registers();
   constexpr auto line = poet::cache_line();

   constexpr auto num_vregs = poet::vector_register_count();
   constexpr auto vwidth_bits = poet::vector_width_bits();
   constexpr auto lanes64 = poet::vector_lanes_64bit();
   constexpr auto lanes32 = poet::vector_lanes_32bit();

Guarantees
----------

- Every value describes the compile target, not the machine that runs the
  binary.

Runnable example
----------------

- Full source: `examples/cpu_info.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/cpu_info.cpp>`_
- |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/cpu_info.html>`_

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
