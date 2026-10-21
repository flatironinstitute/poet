# Installs POET to a scratch prefix, then configures a one-line consumer.
# find_package(POET <this version>) must pass. find_package(POET <next minor>)
# is the negative control: it must fail, or the version file checks nothing.
# Usage: cmake -DPOET_BINARY_DIR=<dir> -DPOET_CONFIG=<config> -DPOET_VERSION=<x.y.z> -DWORK_DIR=<dir> -P install_version_check.cmake
set(prefix "${WORK_DIR}/prefix")
set(consumer "${WORK_DIR}/consumer")
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${consumer}")

execute_process(COMMAND "${CMAKE_COMMAND}" --install "${POET_BINARY_DIR}" --config "${POET_CONFIG}" --prefix "${prefix}" RESULT_VARIABLE rc)
if(rc)
  message(FATAL_ERROR "install failed: ${rc}")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _ "${POET_VERSION}")
math(EXPR next_minor "${CMAKE_MATCH_2} + 1")
set(next "${CMAKE_MATCH_1}.${next_minor}.0")

function(try_consumer request out_rc)
  file(WRITE "${consumer}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.20)\nproject(consumer LANGUAGES NONE)\nfind_package(POET ${request} REQUIRED CONFIG)\n")
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${consumer}" -B "${consumer}/build"
      -DCMAKE_PREFIX_PATH=${prefix}
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  set(${out_rc} ${rc} PARENT_SCOPE)
endfunction()

try_consumer("${POET_VERSION}" rc_same)
if(rc_same)
  message(FATAL_ERROR "find_package(POET ${POET_VERSION}) failed against installed ${POET_VERSION}")
endif()
message(STATUS "find_package(POET ${POET_VERSION}) found the installed package")

try_consumer("${next}" rc_next)
if(NOT rc_next)
  message(FATAL_ERROR "control failed: find_package(POET ${next}) accepted the installed ${POET_VERSION}")
endif()
message(STATUS "control: find_package(POET ${next}) rejected (exit ${rc_next})")
