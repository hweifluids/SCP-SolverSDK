cmake_minimum_required(VERSION 3.24)

if(TEST_FAILURE_MODE STREQUAL "missing-version")
  project(SCPSolverSDKInfoFixture LANGUAGES CXX)
else()
  project(SCPSolverSDKInfoFixture VERSION 1.2.3 LANGUAGES CXX)
endif()

add_library(SCP_SolverSDK INTERFACE)
add_library(SCP::SolverSDK ALIAS SCP_SolverSDK)
add_executable(info_fixture dummy.cpp)
include("${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusSolverInfo.cmake")

if(TEST_FAILURE_MODE STREQUAL "empty-name")
  streamcenterplus_configure_solver_info(info_fixture "" ON OFF)
elseif(TEST_FAILURE_MODE STREQUAL "unsafe-name")
  streamcenterplus_configure_solver_info(info_fixture "bad/name" ON OFF)
elseif(TEST_FAILURE_MODE STREQUAL "missing-version")
  streamcenterplus_configure_solver_info(info_fixture Test ON OFF)
elseif(TEST_FAILURE_MODE STREQUAL "extra-argument")
  streamcenterplus_configure_solver_info(
    info_fixture Test ON OFF structuredmesh single_static unexpected)
elseif(TEST_FAILURE_MODE STREQUAL "invalid-features")
  streamcenterplus_configure_solver_info(
    info_fixture Test ON OFF structuredmesh "single_static,,transient")
elseif(TEST_FAILURE_MODE STREQUAL "invalid-bool")
  streamcenterplus_configure_solver_info(info_fixture Test enabled OFF)
elseif(TEST_FAILURE_MODE STREQUAL "none")
  streamcenterplus_configure_solver_info(
    info_fixture Test ON OFF structuredmesh "single_static,transient")
  get_target_property(_output_name info_fixture OUTPUT_NAME)
  if(NOT _output_name MATCHES
     "^Test_solver_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$")
    message(FATAL_ERROR "Unexpected solver output name: '${_output_name}'.")
  endif()
else()
  message(FATAL_ERROR "Unknown TEST_FAILURE_MODE: '${TEST_FAILURE_MODE}'.")
endif()
