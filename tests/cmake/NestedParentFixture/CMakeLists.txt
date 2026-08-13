cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDKNestedParentFixture VERSION 1.0.0 LANGUAGES CXX)

foreach(_required_variable IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    EXPECT_SDK_TESTS)
  if(NOT DEFINED ${_required_variable} OR "${${_required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${_required_variable} is required.")
  endif()
endforeach()

include(CTest)
if(NOT BUILD_TESTING)
  message(FATAL_ERROR "The nested-parent fixture requires BUILD_TESTING=ON.")
endif()

add_test(
  NAME scp_solver_sdk_nested_parent_sentinel
  COMMAND "${CMAKE_COMMAND}" -E true
)

add_subdirectory("${SCP_SOLVER_SDK_SOURCE_DIR}" solver-sdk EXCLUDE_FROM_ALL)

get_property(_sdk_tests
  DIRECTORY "${SCP_SOLVER_SDK_SOURCE_DIR}"
  PROPERTY TESTS)
list(FIND _sdk_tests scp_solver_sdk_visualization_manifest
  _visualization_test_index)

if(EXPECT_SDK_TESTS)
  if(NOT SCP_SOLVER_SDK_BUILD_TESTING)
    message(FATAL_ERROR
      "Explicit nested enablement did not set SCP_SOLVER_SDK_BUILD_TESTING.")
  endif()
  if(NOT TARGET SCPSolverSDKVisualizationManifestTests)
    message(FATAL_ERROR
      "Explicit nested enablement did not create the SDK test executable.")
  endif()
  get_target_property(_test_exclude_from_all
    SCPSolverSDKVisualizationManifestTests EXCLUDE_FROM_ALL)
  if(NOT _test_exclude_from_all STREQUAL "FALSE")
    message(FATAL_ERROR
      "The explicitly enabled SDK test target remains excluded from the parent ALL target: "
      "${_test_exclude_from_all}")
  endif()
  if(_visualization_test_index EQUAL -1)
    message(FATAL_ERROR
      "Explicit nested enablement did not register the SDK visualization test.")
  endif()
else()
  if(SCP_SOLVER_SDK_BUILD_TESTING)
    message(FATAL_ERROR
      "SCP_SOLVER_SDK_BUILD_TESTING did not default to OFF in a nested project.")
  endif()
  if(TARGET SCPSolverSDKVisualizationManifestTests)
    message(FATAL_ERROR
      "The nested SDK created a test executable without explicit enablement.")
  endif()
  if(NOT _visualization_test_index EQUAL -1 OR _sdk_tests)
    message(FATAL_ERROR
      "The nested SDK registered tests without explicit enablement: ${_sdk_tests}")
  endif()
endif()
