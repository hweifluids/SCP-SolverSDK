cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED SCP_SOLVER_SDK_SOURCE_DIR OR
   SCP_SOLVER_SDK_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "SCP_SOLVER_SDK_SOURCE_DIR is required.")
endif()

# Resolve target-local caps without probing a CUDA compiler or creating a build.
set(STREAMCENTERPLUS_BUILD_MODE "CpuOnly" CACHE STRING "" FORCE)
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusOptionalCuda.cmake")

set(_cap_arguments)
if(DEFINED CAP_ARCHITECTURE_THREADS)
  list(APPEND _cap_arguments
    ARCHITECTURE_THREADS_CAP "${CAP_ARCHITECTURE_THREADS}")
endif()
if(DEFINED CAP_SPLIT_COMPILE_THREADS)
  list(APPEND _cap_arguments
    SPLIT_COMPILE_THREADS_CAP "${CAP_SPLIT_COMPILE_THREADS}")
endif()

_streamcenterplus_resolve_cuda_parallelism(
  _effective_architecture_threads
  _effective_split_compile_threads
  _architecture_threads_cap
  _split_compile_threads_cap
  ${_cap_arguments})

foreach(_required IN ITEMS
    EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS
    EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS
    EXPECTED_ARCHITECTURE_THREADS_CAP
    EXPECTED_SPLIT_COMPILE_THREADS_CAP)
  if(NOT DEFINED "${_required}" OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required.")
  endif()
endforeach()

if(NOT _effective_architecture_threads STREQUAL
       EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS)
  message(FATAL_ERROR
    "Expected effective architecture threads "
    "'${EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS}'; got "
    "'${_effective_architecture_threads}'.")
endif()
if(NOT _effective_split_compile_threads STREQUAL
       EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS)
  message(FATAL_ERROR
    "Expected effective split-compile threads "
    "'${EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS}'; got "
    "'${_effective_split_compile_threads}'.")
endif()
if(NOT _architecture_threads_cap STREQUAL
       EXPECTED_ARCHITECTURE_THREADS_CAP)
  message(FATAL_ERROR
    "Expected architecture thread cap '${EXPECTED_ARCHITECTURE_THREADS_CAP}'; "
    "got '${_architecture_threads_cap}'.")
endif()
if(NOT _split_compile_threads_cap STREQUAL
       EXPECTED_SPLIT_COMPILE_THREADS_CAP)
  message(FATAL_ERROR
    "Expected split-compile thread cap "
    "'${EXPECTED_SPLIT_COMPILE_THREADS_CAP}'; got "
    "'${_split_compile_threads_cap}'.")
endif()

message(STATUS
  "CUDA cap validation passed: "
  "requested=${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}/"
  "${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}; "
  "cap=${_architecture_threads_cap}/${_split_compile_threads_cap}; "
  "effective=${_effective_architecture_threads}/"
  "${_effective_split_compile_threads}.")
