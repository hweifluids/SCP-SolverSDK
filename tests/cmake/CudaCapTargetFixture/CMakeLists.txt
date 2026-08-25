cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDKCudaCapTargetFixture LANGUAGES CXX)

foreach(_required IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    TEST_HELPER
    EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS
    EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS
    EXPECTED_ARCHITECTURE_THREADS_CAP
    EXPECTED_SPLIT_COMPILE_THREADS_CAP)
  if(NOT DEFINED "${_required}" OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required.")
  endif()
endforeach()
if(NOT TEST_HELPER STREQUAL "apply" AND
   NOT TEST_HELPER STREQUAL "add")
  message(FATAL_ERROR "TEST_HELPER must be 'apply' or 'add'.")
endif()
if(NOT DEFINED TEST_SEPARABLE_MODE OR TEST_SEPARABLE_MODE STREQUAL "")
  set(TEST_SEPARABLE_MODE "default")
endif()
if(NOT TEST_SEPARABLE_MODE MATCHES "^(default|explicit-on|explicit-off)$")
  message(FATAL_ERROR
    "TEST_SEPARABLE_MODE must be default, explicit-on, or explicit-off.")
endif()
if(NOT DEFINED EXPECTED_SEPARABLE OR EXPECTED_SEPARABLE STREQUAL "")
  set(EXPECTED_SEPARABLE OFF)
endif()
if(NOT EXPECTED_SEPARABLE MATCHES "^(ON|OFF)$")
  message(FATAL_ERROR "EXPECTED_SEPARABLE must be ON or OFF.")
endif()
if(NOT DEFINED TEST_ARGUMENT_FAILURE_MODE OR
   TEST_ARGUMENT_FAILURE_MODE STREQUAL "")
  set(TEST_ARGUMENT_FAILURE_MODE "none")
endif()
if(NOT TEST_ARGUMENT_FAILURE_MODE MATCHES
   "^(none|unknown-before-sources|unknown-after-sources|misspelled-separable-after-sources|unknown-after-libraries|missing-cap-value|conflicting-separable)$")
  message(FATAL_ERROR
    "TEST_ARGUMENT_FAILURE_MODE must be none, unknown-before-sources, "
    "unknown-after-sources, misspelled-separable-after-sources, "
    "unknown-after-libraries, missing-cap-value, or conflicting-separable.")
endif()

set(STREAMCENTERPLUS_BUILD_MODE "CpuOnly" CACHE STRING "" FORCE)
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusOptionalCuda.cmake")

if(TEST_CUDA_CACHE AND
   (NOT DEFINED TEST_CUDA_CACHE_MODE OR TEST_CUDA_CACHE_MODE STREQUAL ""))
  set(TEST_CUDA_CACHE_MODE "valid")
endif()
if(NOT DEFINED TEST_CUDA_CACHE_MODE OR TEST_CUDA_CACHE_MODE STREQUAL "")
  set(TEST_CUDA_CACHE_MODE "none")
endif()
if(NOT TEST_CUDA_CACHE_MODE MATCHES "^(none|valid|wrong_name|missing)$")
  message(FATAL_ERROR
    "TEST_CUDA_CACHE_MODE must be none, valid, wrong_name, or missing.")
endif()

set(_fixture_cuda_cache_enabled OFF)
if(TEST_CUDA_CACHE_MODE STREQUAL "valid")
  set(_fixture_cuda_cache_shim "${CMAKE_CURRENT_BINARY_DIR}/nvcc.exe")
  file(WRITE "${_fixture_cuda_cache_shim}" "fixture")
  set(ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM}
      "${_fixture_cuda_cache_shim}")
  set(_fixture_cuda_cache_enabled ON)
elseif(TEST_CUDA_CACHE_MODE STREQUAL "wrong_name")
  set(_fixture_cuda_cache_shim "${CMAKE_CURRENT_BINARY_DIR}/ccache.exe")
  file(WRITE "${_fixture_cuda_cache_shim}" "fixture")
  set(ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM}
      "${_fixture_cuda_cache_shim}")
elseif(TEST_CUDA_CACHE_MODE STREQUAL "missing")
  set(_fixture_cuda_cache_shim "${CMAKE_CURRENT_BINARY_DIR}/nvcc.exe")
  file(REMOVE "${_fixture_cuda_cache_shim}")
  set(ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM}
      "${_fixture_cuda_cache_shim}")
else()
  unset(ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM})
endif()

# Exercise target configuration without enabling CUDA or compiling anything.
set(STREAMCENTERPLUS_CUDA_AVAILABLE ON)
set(_cap_arguments)
if(DEFINED CAP_ARCHITECTURE_THREADS)
  list(APPEND _cap_arguments
    ARCHITECTURE_THREADS_CAP "${CAP_ARCHITECTURE_THREADS}")
endif()
if(DEFINED CAP_SPLIT_COMPILE_THREADS)
  list(APPEND _cap_arguments
    SPLIT_COMPILE_THREADS_CAP "${CAP_SPLIT_COMPILE_THREADS}")
endif()

if(TEST_HELPER STREQUAL "apply")
  add_library(cuda_cap_fixture STATIC dummy.cpp)
  streamcenterplus_apply_cuda_release_codegen(
    cuda_cap_fixture ${_cap_arguments})
else()
  add_library(cuda_cap_fixture STATIC)
  set(_separable_arguments)
  if(TEST_SEPARABLE_MODE STREQUAL "explicit-on")
    list(APPEND _separable_arguments SEPARABLE)
  elseif(TEST_SEPARABLE_MODE STREQUAL "explicit-off")
    list(APPEND _separable_arguments NO_SEPARABLE)
  endif()
  if(TEST_ARGUMENT_FAILURE_MODE STREQUAL "unknown-before-sources")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      UNKNOWN_CUDA_POLICY
      SOURCES dummy.cpp
    )
  elseif(TEST_ARGUMENT_FAILURE_MODE STREQUAL "unknown-after-sources")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      SOURCES dummy.cpp UNKNOWN_CUDA_POLICY
    )
  elseif(TEST_ARGUMENT_FAILURE_MODE STREQUAL "misspelled-separable-after-sources")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      SOURCES dummy.cpp SEPERABLE
    )
  elseif(TEST_ARGUMENT_FAILURE_MODE STREQUAL "unknown-after-libraries")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      SOURCES dummy.cpp
      LIBRARIES fixture_dependency UNKNOWN_CUDA_POLICY
    )
  elseif(TEST_ARGUMENT_FAILURE_MODE STREQUAL "missing-cap-value")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      ARCHITECTURE_THREADS_CAP
      SOURCES dummy.cpp
    )
  elseif(TEST_ARGUMENT_FAILURE_MODE STREQUAL "conflicting-separable")
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      SEPARABLE
      NO_SEPARABLE
      SOURCES dummy.cpp
    )
  else()
    streamcenterplus_add_optional_cuda(
      cuda_cap_fixture
      ${_cap_arguments}
      ${_separable_arguments}
      SOURCES dummy.cpp
    )
  endif()
endif()

function(_assert_target_property property_name expected_value)
  get_target_property(_actual cuda_cap_fixture "${property_name}")
  if(NOT _actual STREQUAL expected_value)
    message(FATAL_ERROR
      "Expected target property ${property_name}='${expected_value}'; "
      "got '${_actual}'.")
  endif()
endfunction()

_assert_target_property(
  STREAMCENTERPLUS_CUDA_EFFECTIVE_ARCHITECTURE_THREADS
  "${EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS}")
_assert_target_property(
  STREAMCENTERPLUS_CUDA_EFFECTIVE_SPLIT_COMPILE_THREADS
  "${EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS}")
_assert_target_property(
  STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS_CAP
  "${EXPECTED_ARCHITECTURE_THREADS_CAP}")
_assert_target_property(
  STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS_CAP
  "${EXPECTED_SPLIT_COMPILE_THREADS_CAP}")
if(_fixture_cuda_cache_enabled)
  _assert_target_property(
    STREAMCENTERPLUS_CUDA_COMPILER_CACHE_ENABLED ON)
  file(TO_CMAKE_PATH
    "${_fixture_cuda_cache_shim}"
    _fixture_cuda_cache_shim_normalized)
  _assert_target_property(
    STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM
    "${_fixture_cuda_cache_shim_normalized}")
  _assert_target_property(
    VS_GLOBAL_CudaToolkitNvccPath
    "${_fixture_cuda_cache_shim_normalized}")
else()
  _assert_target_property(
    STREAMCENTERPLUS_CUDA_COMPILER_CACHE_ENABLED OFF)
endif()
if(TEST_HELPER STREQUAL "add")
  _assert_target_property(
    CUDA_SEPARABLE_COMPILATION "${EXPECTED_SEPARABLE}")
  _assert_target_property(
    CUDA_RESOLVE_DEVICE_SYMBOLS "${EXPECTED_SEPARABLE}")
endif()

get_target_property(_compile_options cuda_cap_fixture COMPILE_OPTIONS)
set(_architecture_flag_count 0)
set(_split_compile_flag_count 0)
foreach(_option IN LISTS _compile_options)
  if(_option MATCHES "--threads=([^>]+)")
    math(EXPR _architecture_flag_count "${_architecture_flag_count} + 1")
    set(_architecture_flag_value "${CMAKE_MATCH_1}")
  endif()
  if(_option MATCHES "--split-compile=([^>]+)")
    math(EXPR _split_compile_flag_count "${_split_compile_flag_count} + 1")
    set(_split_compile_flag_value "${CMAKE_MATCH_1}")
  endif()
  if(_option MATCHES "--threads=0" OR
     _option MATCHES "--split-compile=0")
    message(FATAL_ERROR "CUDA target options must never contain a zero thread flag.")
  endif()
endforeach()

if(EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS STREQUAL "1")
  if(NOT _architecture_flag_count EQUAL 0)
    message(FATAL_ERROR
      "Serial architecture compilation must omit --threads; got "
      "'${_compile_options}'.")
  endif()
elseif(NOT _architecture_flag_count EQUAL 1 OR
       NOT _architecture_flag_value STREQUAL
         EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS)
  message(FATAL_ERROR
    "Expected one --threads=${EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS}; got "
    "'${_compile_options}'.")
endif()

if(EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS STREQUAL "1")
  if(NOT _split_compile_flag_count EQUAL 0)
    message(FATAL_ERROR
      "Serial split compilation must omit --split-compile; got "
      "'${_compile_options}'.")
  endif()
elseif(NOT _split_compile_flag_count EQUAL 1 OR
       NOT _split_compile_flag_value STREQUAL
         EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS)
  message(FATAL_ERROR
    "Expected one "
    "--split-compile=${EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS}; got "
    "'${_compile_options}'.")
endif()

message(STATUS
  "CUDA target cap fixture passed: helper=${TEST_HELPER}; "
  "requested=${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}/"
  "${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}; "
  "cap=${EXPECTED_ARCHITECTURE_THREADS_CAP}/"
  "${EXPECTED_SPLIT_COMPILE_THREADS_CAP}; "
  "effective=${EXPECTED_EFFECTIVE_ARCHITECTURE_THREADS}/"
  "${EXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS}.")
