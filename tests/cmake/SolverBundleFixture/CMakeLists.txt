cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDKSolverBundleFixture LANGUAGES CXX)

if(NOT DEFINED SCP_SOLVER_SDK_SOURCE_DIR OR
   "${SCP_SOLVER_SDK_SOURCE_DIR}" STREQUAL "")
  message(FATAL_ERROR "SCP_SOLVER_SDK_SOURCE_DIR is required.")
endif()
if(NOT DEFINED TEST_MODE OR
   NOT TEST_MODE MATCHES "^(default|bare-destination)$")
  message(FATAL_ERROR "TEST_MODE must be default or bare-destination.")
endif()

add_executable(bundle_fixture dummy.cpp)
include(
  "${SCP_SOLVER_SDK_SOURCE_DIR}/cmake/modules/StreamcenterPlusSolverBundle.cmake")

if(TEST_MODE STREQUAL "bare-destination")
  streamcenterplus_install_solver_bundle(bundle_fixture DESTINATION)
else()
  streamcenterplus_install_solver_bundle(bundle_fixture)
endif()
