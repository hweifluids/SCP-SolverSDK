cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDKSolverInfoCudaFixture VERSION 1.2.3 LANGUAGES CXX)

if(NOT DEFINED SCP_SOLVER_SDK_SOURCE_DIR OR
   "${SCP_SOLVER_SDK_SOURCE_DIR}" STREQUAL "")
  message(FATAL_ERROR "SCP_SOLVER_SDK_SOURCE_DIR is required.")
endif()
if(NOT DEFINED TEST_MODE OR
   NOT TEST_MODE MATCHES "^(matching|matching-cpu|direct-mismatch|direct-mismatch-reversed|transitive-mismatch)$")
  message(FATAL_ERROR
    "TEST_MODE must be matching, matching-cpu, direct-mismatch, "
    "direct-mismatch-reversed, or transitive-mismatch.")
endif()

add_library(SCP_SolverSDK INTERFACE)
target_include_directories(SCP_SolverSDK INTERFACE
  "${SCP_SOLVER_SDK_SOURCE_DIR}/include")
add_library(SCP::SolverSDK ALIAS SCP_SolverSDK)

set(STREAMCENTERPLUS_BUILD_MODE "CpuOnly" CACHE STRING "" FORCE)
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusOptionalCuda.cmake")
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusSolverInfo.cmake")

# Exercise OptionalCuda's target contract without requiring a CUDA compiler.
set(_fixture_cuda_capability 1)
if(TEST_MODE STREQUAL "matching-cpu")
  set(_fixture_cuda_capability 0)
endif()
set(STREAMCENTERPLUS_CUDA_AVAILABLE "${_fixture_cuda_capability}")
add_library(cuda_core STATIC cuda_core.cpp)
streamcenterplus_add_optional_cuda(cuda_core SOURCES cuda_core.cpp)

add_executable(info_fixture dummy.cpp)
target_compile_definitions(info_fixture PRIVATE
  SCP_SOLVER_SDK_EXPECTED_CUDA=${_fixture_cuda_capability})
if(TEST_MODE STREQUAL "direct-mismatch")
  streamcenterplus_configure_solver_info(cuda_core Test ON OFF)
elseif(TEST_MODE STREQUAL "direct-mismatch-reversed")
  streamcenterplus_configure_solver_info(info_fixture Test ON OFF)
  streamcenterplus_add_optional_cuda(info_fixture SOURCES dummy.cpp)
elseif(TEST_MODE STREQUAL "transitive-mismatch")
  streamcenterplus_configure_solver_info(info_fixture Test ON OFF)
  target_link_libraries(info_fixture PRIVATE cuda_core)
else()
  streamcenterplus_configure_solver_info(
    info_fixture Test ON "${_fixture_cuda_capability}")
  target_link_libraries(info_fixture PRIVATE cuda_core)

  get_target_property(_info_compile_definitions info_fixture COMPILE_DEFINITIONS)
  list(FIND _info_compile_definitions
    "STREAMCENTERPLUS_SOLVER_HAS_CUDA=${_fixture_cuda_capability}"
    _solver_metadata_definition)
  if(_solver_metadata_definition EQUAL -1)
    message(FATAL_ERROR
      "Solver-info metadata did not define STREAMCENTERPLUS_SOLVER_HAS_CUDA="
      "${_fixture_cuda_capability}.")
  endif()
  foreach(_definition IN LISTS _info_compile_definitions)
    if(_definition MATCHES "^STREAMCENTERPLUS_HAS_CUDA=")
      message(FATAL_ERROR
        "Solver-info target must not directly define the CUDA code-generation macro: "
        "${_definition}.")
    endif()
  endforeach()

  get_target_property(_core_interface_definitions cuda_core
    INTERFACE_COMPILE_DEFINITIONS)
  list(FIND _core_interface_definitions
    "STREAMCENTERPLUS_HAS_CUDA=${_fixture_cuda_capability}"
    _core_capability_definition)
  if(_core_capability_definition EQUAL -1)
    message(FATAL_ERROR
      "OptionalCuda did not publish STREAMCENTERPLUS_HAS_CUDA="
      "${_fixture_cuda_capability} to consumers.")
  endif()
endif()
