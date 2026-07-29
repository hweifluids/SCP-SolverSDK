cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED SCP_SOLVER_SDK_SOURCE_DIR OR
   SCP_SOLVER_SDK_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "SCP_SOLVER_SDK_SOURCE_DIR is required.")
endif()
if(NOT DEFINED EXPECTED_SPLIT_COMPILE_THREADS OR
   EXPECTED_SPLIT_COMPILE_THREADS STREQUAL "")
  message(FATAL_ERROR "EXPECTED_SPLIT_COMPILE_THREADS is required.")
endif()
if(NOT DEFINED EXPECTED_ARCHITECTURE_THREADS OR
   EXPECTED_ARCHITECTURE_THREADS STREQUAL "")
  set(EXPECTED_ARCHITECTURE_THREADS "12")
endif()

# This validates cache defaults and external overrides without probing a CUDA
# compiler or starting a CUDA build.
set(STREAMCENTERPLUS_BUILD_MODE "CpuOnly" CACHE STRING "" FORCE)
if(DEFINED NORMAL_ARCHITECTURE_THREADS)
  set(STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS
      "${NORMAL_ARCHITECTURE_THREADS}")
endif()
if(DEFINED NORMAL_SPLIT_COMPILE_THREADS)
  set(STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS
      "${NORMAL_SPLIT_COMPILE_THREADS}")
endif()
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusOptionalCuda.cmake")

set(_expected_public_architectures
  "75-real;80-real;86-real;87-real;88-real;89-real;90-real;100-real;103-real;110-real;120-real;121-real;120-virtual")
set(_expected_cmake_architectures
  "75-real;80-real;86-real;87-real;88-real;89-real;90-real;100-real;103-real;110-real")
set(_expected_blackwell_codegen
  "--generate-code=arch=compute_120,code=[sm_120,sm_121,compute_120]")

if(NOT STREAMCENTERPLUS_CUDA_ARCHITECTURES STREQUAL
       _expected_public_architectures)
  message(FATAL_ERROR
    "Published CUDA architecture matrix changed: "
    "'${STREAMCENTERPLUS_CUDA_ARCHITECTURES}'.")
endif()
if(NOT STREAMCENTERPLUS_CMAKE_CUDA_ARCHITECTURES STREQUAL
       _expected_cmake_architectures)
  message(FATAL_ERROR
    "CMake CUDA architecture matrix changed: "
    "'${STREAMCENTERPLUS_CMAKE_CUDA_ARCHITECTURES}'.")
endif()
if(NOT STREAMCENTERPLUS_CUDA_COMBINED_BLACKWELL_CODEGEN STREQUAL
       _expected_blackwell_codegen)
  message(FATAL_ERROR
    "Combined Blackwell code generation changed: "
    "'${STREAMCENTERPLUS_CUDA_COMBINED_BLACKWELL_CODEGEN}'.")
endif()
if(NOT STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS STREQUAL
       EXPECTED_ARCHITECTURE_THREADS)
  message(FATAL_ERROR
    "Expected architecture threads '${EXPECTED_ARCHITECTURE_THREADS}'; got "
    "'${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}'.")
endif()
if(NOT STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS STREQUAL
       EXPECTED_SPLIT_COMPILE_THREADS)
  message(FATAL_ERROR
    "Expected split-compile override '${EXPECTED_SPLIT_COMPILE_THREADS}'; got "
    "'${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}'.")
endif()

foreach(_thread_cache IN ITEMS
    STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS
    STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS)
  get_property(_thread_cache_type CACHE "${_thread_cache}" PROPERTY TYPE)
  if(NOT _thread_cache_type STREQUAL "STRING")
    message(FATAL_ERROR
      "${_thread_cache} must be normalized to a STRING cache entry; got "
      "'${_thread_cache_type}'.")
  endif()
endforeach()

get_property(
  _split_compile_choices
  CACHE STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS
  PROPERTY STRINGS)
foreach(_benchmark_value IN ITEMS 2 4 8)
  list(FIND _split_compile_choices "${_benchmark_value}" _benchmark_index)
  if(_benchmark_index EQUAL -1)
    message(FATAL_ERROR
      "Split-compile benchmark value ${_benchmark_value} is not exposed in "
      "the cache choices '${_split_compile_choices}'.")
  endif()
endforeach()
