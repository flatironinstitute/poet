include_guard(GLOBAL)

# ==============================================================================
# POET Development Helpers
# ==============================================================================
# This file provides CMake helper functions and targets for POET development:
# - Compiler warnings configuration (poet_enable_warnings)
# - Sanitizers setup (poet_enable_sanitizers)
# - Static analysis tools (poet_configure_static_analysis)
# - Documentation generation (doxygen, sphinx, docs targets)
# - Code coverage reporting (coverage target)
#
# These helpers are only used for development/testing builds and are not
# required when using POET as a header-only library.
# ==============================================================================

# Prepare CPM (CMake Package Manager) for fetching dependencies
# Note: This is only used for developer/test builds, not for header-only library consumers
include(FetchContent)

FetchContent_Declare(
  CPM
  URL https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
  URL_HASH SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
  DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_GetProperties(CPM)
if(NOT CPM_POPULATED)
  FetchContent_MakeAvailable(CPM)
endif()

include(${cpm_SOURCE_DIR}/CPM.cmake)

# -------------------------
# Warnings helper (from PoetWarnings.cmake)
# -------------------------
option(POET_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Enable comprehensive compiler warnings for a target
# Supports GCC, Clang, AppleClang, and MSVC compilers
# Applies warnings with PRIVATE scope for regular targets, INTERFACE scope for interface libraries
function(poet_enable_warnings target)
  if (NOT TARGET "${target}")
    message(FATAL_ERROR "poet_enable_warnings called with non-existent target '${target}'")
  endif()

  # Determine the appropriate scope for applying warnings
  # INTERFACE scope for interface libraries (warnings propagate to consumers)
  # PRIVATE scope for other targets (warnings only apply to this target's sources)
  get_target_property(_target_type "${target}" TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_scope INTERFACE)
  else()
    set(_scope PRIVATE)
  endif()

  # Generator expressions for compiler detection (evaluated at build time)
  set(_clang_like $<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
  set(_gnu $<CXX_COMPILER_ID:GNU>)
  set(_gnu_or_clang $<OR:${_gnu},${_clang_like}>)
  set(_msvc $<CXX_COMPILER_ID:MSVC>)
  set(_lang_is_cxx $<COMPILE_LANGUAGE:CXX>)

  set(_warnings_clang_like
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wold-style-cast
    -Wnon-virtual-dtor
    -Wnull-dereference
    -Woverloaded-virtual
    -Wcast-align
    -Wunused
    -Wimplicit-fallthrough
    -Wformat=2
  )

  # Additional curated warnings that are checked for compiler support before enabling
  # These are only added if the compiler supports them (GCC-specific flags)
  set(_additional_warnings
    -Wduplicated-cond
    -Wlogical-op
    -Wuseless-cast
    -Winit-self
    -Wmissing-include-dirs
    -Wredundant-decls
  )

  # Check which additional warnings are supported by the current compiler and add them
  include(CheckCXXCompilerFlag)
  foreach(_f IN LISTS _additional_warnings)
    check_cxx_compiler_flag("${_f}" _flag_supported)
    if(_flag_supported)
      list(APPEND _warnings_clang_like ${_f})
    endif()
  endforeach()

  set(_warnings_gnu_only
    -Wmisleading-indentation
    -Wsuggest-override
  )

  set(_warnings_msvc
    /W4
    /permissive-
    /bigobj
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
  )

  # Build compiler-specific warning flags using generator expressions
  # Flags are only applied when compiling C++ code with the matching compiler
  set(_compile_options)
  foreach(flag IN LISTS _warnings_clang_like)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu_or_clang}>:${flag}>)
  endforeach()
  foreach(flag IN LISTS _warnings_gnu_only)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu}>:${flag}>)
  endforeach()
  foreach(flag IN LISTS _warnings_msvc)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_msvc}>:${flag}>)
  endforeach()

  # Add -Werror / /WX if treating warnings as errors
  if(POET_WARNINGS_AS_ERRORS)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu_or_clang}>:-Werror>)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_msvc}>:/WX>)
  endif()

  target_compile_options(${target} ${_scope} ${_compile_options})
endfunction()

# -------------------------
# Sanitizers helper (from PoetSanitizers.cmake)
# -------------------------
option(POET_ENABLE_SANITIZERS "Master switch to enable all sanitizers at once" OFF)

option(POET_ENABLE_ASAN "Enable AddressSanitizer (memory error detection)" ${POET_ENABLE_SANITIZERS})
option(POET_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (undefined behavior detection)" ${POET_ENABLE_SANITIZERS})

# Applies sanitizer flags to a target. Flags are set directly rather than via
# arsenm/sanitizers-cmake: that module's find_package() ran inside a function, so
# the ASan_*_FLAGS variables it defines died with that scope and add_sanitizers()
# silently applied nothing -- POET_ENABLE_SANITIZERS=ON built with no -fsanitize
# at all. Two flag strings do not warrant a downloaded dependency.
#
# -fno-sanitize-recover=all matters as much as the sanitizers themselves: without
# it UBSan prints a diagnostic and carries on, so ctest still reports success.
function(poet_enable_sanitizers target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_enable_sanitizers called with non-existent target '${target}'")
  endif()

  set(_sanitizers)
  if(POET_ENABLE_ASAN)
    list(APPEND _sanitizers address)
  endif()
  if(POET_ENABLE_UBSAN)
    list(APPEND _sanitizers undefined)
  endif()
  if(NOT _sanitizers)
    return()
  endif()

  if(MSVC)
    # cl.exe only implements AddressSanitizer, and it takes no link flag.
    if(POET_ENABLE_ASAN)
      set(_flags /fsanitize=address)
    else()
      message(WARNING "POET: MSVC has no UndefinedBehaviorSanitizer; UBSan request ignored")
      return()
    endif()
    set(_link_flags)
  else()
    string(JOIN "," _list ${_sanitizers})
    set(_flags -fsanitize=${_list} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    set(_link_flags -fsanitize=${_list})
  endif()

  get_target_property(_target_type "${target}" TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_scope INTERFACE)
  else()
    set(_scope PRIVATE)
  endif()

  target_compile_options(${target} ${_scope} ${_flags})
  if(_link_flags)
    target_link_options(${target} ${_scope} ${_link_flags})
  endif()

  # Record what was really applied, so poet_print_summary() reports effective
  # state rather than the requested options.
  set_property(GLOBAL PROPERTY POET_APPLIED_SANITIZERS "${_flags}")
  set_property(GLOBAL APPEND PROPERTY POET_SANITIZED_TARGETS "${target}")
endfunction()

# -------------------------
# Static analysis helper (from PoetStaticAnalysis.cmake)
# -------------------------
option(POET_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" ON)
# STRING, not option(): option() defaults are booleans, so a string default
# collapses to OFF and the value is silently lost.
set(POET_CLANG_TIDY_CHECKS "" CACHE STRING
  "Override default clang-tidy checks (leave empty to use the .clang-tidy config)")
option(POET_CLANG_TIDY_WARNINGS_AS_ERRORS "Treat clang-tidy warnings as errors" ON)
option(POET_ENABLE_CPPCHECK "Enable cppcheck static analysis" ON)
set(POET_CPPCHECK_OPTIONS "--enable=warning,style,performance,portability" CACHE STRING
  "Additional cppcheck options")

# Configure static analysis tools (clang-tidy and/or cppcheck) for a target
# Tools are only enabled if found on PATH, otherwise a warning is issued
function(poet_configure_static_analysis target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_configure_static_analysis called with non-existent target '${target}'")
  endif()

  if(POET_ENABLE_CLANG_TIDY)
    find_program(_clang_tidy_exe NAMES clang-tidy clang-tidy-17 clang-tidy-16)
    if(_clang_tidy_exe)
      set(_clang_tidy_command "${_clang_tidy_exe}")
      # When POET_CLANG_TIDY_CHECKS is set, use those checks; otherwise let
      # clang-tidy pick up the .clang-tidy config file from the source tree.
      if(POET_CLANG_TIDY_CHECKS)
        set(_clang_tidy_command "${_clang_tidy_command};-checks=${POET_CLANG_TIDY_CHECKS}")
      endif()
      # Only analyze headers in the project's include/poet and src directories (not external dependencies)
      set(_clang_tidy_command "${_clang_tidy_command};-header-filter=^${PROJECT_SOURCE_DIR}/(include/poet|src)")
      # Speed up analysis by only checking syntax, not generating code
      set(_clang_tidy_command "${_clang_tidy_command};--extra-arg=-fsyntax-only")
      if(POET_CLANG_TIDY_WARNINGS_AS_ERRORS)
        set(_clang_tidy_command "${_clang_tidy_command};-warnings-as-errors=*")
      endif()
      set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY "${_clang_tidy_command}")
      set_property(GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED "${_clang_tidy_exe}")
    else()
      message(WARNING "POET_ENABLE_CLANG_TIDY is ON but clang-tidy was not found on PATH")
      set_property(GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED "NOT FOUND")
    endif()
  endif()

  if(POET_ENABLE_CPPCHECK)
    find_program(_cppcheck_exe NAMES cppcheck)
    if(_cppcheck_exe)
      set(_cppcheck_command "${_cppcheck_exe};--inline-suppr;${POET_CPPCHECK_OPTIONS}")
      set_property(TARGET ${target} PROPERTY CXX_CPPCHECK "${_cppcheck_command}")
      set_property(GLOBAL PROPERTY POET_CPPCHECK_RESOLVED "${_cppcheck_exe}")
    else()
      message(WARNING "POET_ENABLE_CPPCHECK is ON but cppcheck was not found on PATH")
      set_property(GLOBAL PROPERTY POET_CPPCHECK_RESOLVED "NOT FOUND")
    endif()
  endif()
endfunction()

# -------------------------
# Docs helper (from PoetDocs.cmake)
# -------------------------
option(POET_GENERATE_DOCS "Generate documentation using Doxygen + Sphinx pipeline" OFF)

if(POET_GENERATE_DOCS)
    # Require Doxygen for API documentation extraction
    find_package(Doxygen REQUIRED)
    # Require Sphinx for generating HTML documentation
    find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build REQUIRED)
    # Require Python for Sphinx and its extensions
    find_package(Python COMPONENTS Interpreter REQUIRED)

    # Check if required Python packages (breathe, exhale) are installed
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import breathe, exhale"
        RESULT_VARIABLE DOCS_DEPS_CHECK_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT DOCS_DEPS_CHECK_RESULT EQUAL 0)
        message(WARNING "Python packages 'breathe' and 'exhale' not found. Docs generation may fail. Please run 'pip install -r docs/requirements.txt'.")
    endif()

    # Generate Doxyfile from template
    configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_BINARY_DIR}/docs/Doxyfile @ONLY)

    # Target: Generate Doxygen XML output from source code
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs
        COMMENT "Generating API documentation with Doxygen"
    )

    # Target: Generate HTML documentation from Doxygen XML using Sphinx
    add_custom_target(sphinx
        DEPENDS doxygen
        COMMAND ${CMAKE_COMMAND} -E env DOXYGEN_XML_OUTPUT=${CMAKE_BINARY_DIR}/docs/xml
                ${SPHINX_BUILD_EXECUTABLE} -b html
                ${CMAKE_SOURCE_DIR}/docs ${CMAKE_BINARY_DIR}/docs/_build/html
        COMMENT "Generating HTML documentation with Sphinx"
    )

    # Target: Complete documentation build (alias for sphinx target)
    add_custom_target(docs DEPENDS sphinx)
    message(STATUS "POET: Documentation targets enabled (doxygen, sphinx, docs)")
endif()

# -------------------------
# Coverage target (moved from top-level)
# -------------------------
# Creates a `coverage` custom target that:
# 1. Builds all test executables
# 2. Runs the test suite using CTest
# 3. Collects code coverage data
# 4. Generates an HTML coverage report
#
# Prefers lcov+genhtml (more robust), falls back to gcovr if unavailable
find_program(GCOVR_EXECUTABLE gcovr)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

# Prefer lcov+genhtml when available (generally more robust and handles complex build trees better)
# Falls back to gcovr if lcov/genhtml are not found
if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
  set(LCOV_INFO ${CMAKE_BINARY_DIR}/coverage.info)
  set(LCOV_FILTERED ${CMAKE_BINARY_DIR}/coverage.filtered.info)
  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${LCOV_INFO} --ignore-errors inconsistent,unused
    # Remove system headers (/usr/*) and CMake FetchContent dependencies (*/_deps/*)
    # to focus coverage reports on project sources only
    COMMAND ${LCOV_EXECUTABLE} --remove ${LCOV_INFO} "/usr/*" "*/_deps/*" --output-file ${LCOV_FILTERED} --ignore-errors inconsistent,unused
    COMMAND ${GENHTML_EXECUTABLE} -o ${COVERAGE_DIR} ${LCOV_FILTERED}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (lcov+genhtml) -> ${COVERAGE_DIR}/index.html"
    VERBATIM
  )
elseif(GCOVR_EXECUTABLE)
  # Fallback to gcovr if lcov/genhtml aren't available
  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    # Filter to project sources (include/poet and tests), excluding external dependencies and system headers
    COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_SOURCE_DIR} --filter "include/poet/|tests/" --exclude ".*/_deps/.*" --exclude "/usr/.*" --gcov-ignore-errors=no_working_dir_found --html --html-details -o ${CMAKE_BINARY_DIR}/coverage-report.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (gcovr) -> ${CMAKE_BINARY_DIR}/coverage-report.html"
    VERBATIM
  )
else()
  message(STATUS "Coverage tools not found: install 'lcov'+'genhtml' or 'gcovr' to enable the 'coverage' target.")
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage tools missing. Install 'lcov'+'genhtml' or 'gcovr' and re-run CMake to enable coverage generation."
  )
endif()

# Ensure coverage target builds all test executables before running CTest
# This adds dependencies on all C++ standard-specific test targets (C++17, C++20, C++23)
if(TARGET coverage)
  set(POET_TEST_STANDARDS 23 20 17)
  foreach(POET_STD IN LISTS POET_TEST_STANDARDS)
    if(TARGET poet_tests_std${POET_STD})
      add_dependencies(coverage poet_tests_std${POET_STD})
    endif()
  endforeach()
  # Also add dependency on the main test target if it exists
  if(TARGET poet_tests)
    add_dependencies(coverage poet_tests)
  endif()
endif()

# -------------------------
# Configuration summary
# -------------------------
# Reports EFFECTIVE state, not requested options. A tool that was asked for but
# silently did nothing (missing binary, unsupported compiler, a helper that
# returned early) is the failure mode this exists to make visible -- otherwise
# the only way to tell is to grep compile_commands.json.
function(poet_print_summary)
  get_property(_san GLOBAL PROPERTY POET_APPLIED_SANITIZERS)
  get_property(_san_targets GLOBAL PROPERTY POET_SANITIZED_TARGETS)
  get_property(_tidy GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED)
  get_property(_cppcheck GLOBAL PROPERTY POET_CPPCHECK_RESOLVED)

  message(STATUS "")
  message(STATUS "── POET ${POET_VERSION_FULL} ──────────────────────────────")
  message(STATUS "  compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "  build type      : ${CMAKE_BUILD_TYPE}")
  message(STATUS "  tests/examples/benchmarks : ${POET_BUILD_TESTS}/${POET_BUILD_EXAMPLES}/${POET_BUILD_BENCHMARKS}")
  message(STATUS "  strict warnings : ${POET_STRICT_WARNINGS}")

  if(_san)
    list(LENGTH _san_targets _n)
    message(STATUS "  sanitizers      : ACTIVE on ${_n} target(s) -- ${_san}")
  elseif(POET_ENABLE_ASAN OR POET_ENABLE_UBSAN)
    message(WARNING "POET: sanitizers requested but no flags were applied to any target")
  else()
    message(STATUS "  sanitizers      : off")
  endif()

  foreach(_tool tidy cppcheck)
    if(_tool STREQUAL tidy)
      set(_want ${POET_ENABLE_CLANG_TIDY})
      set(_got "${_tidy}")
      set(_label "clang-tidy      ")
    else()
      set(_want ${POET_ENABLE_CPPCHECK})
      set(_got "${_cppcheck}")
      set(_label "cppcheck        ")
    endif()
    if(NOT _want)
      message(STATUS "  ${_label}: off")
    elseif(_got STREQUAL "NOT FOUND" OR _got STREQUAL "")
      message(STATUS "  ${_label}: REQUESTED BUT INACTIVE")
    else()
      message(STATUS "  ${_label}: ${_got}")
    endif()
  endforeach()
  message(STATUS "─────────────────────────────────────────────────────────")
  message(STATUS "")
endfunction()
