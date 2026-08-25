cmake_minimum_required(VERSION 3.24)

foreach(_required IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    FIXTURE_SOURCE_DIR
    FIXTURE_BINARY_DIR
    TEST_GENERATOR
    TEST_MODE)
  if(NOT DEFINED "${_required}" OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required.")
  endif()
endforeach()

file(REMOVE_RECURSE "${FIXTURE_BINARY_DIR}")
set(_configure_command
  "${CMAKE_COMMAND}"
  -S "${FIXTURE_SOURCE_DIR}"
  -B "${FIXTURE_BINARY_DIR}"
  -G "${TEST_GENERATOR}"
  "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release"
  "-DSCP_SOLVER_SDK_SOURCE_DIR=${SCP_SOLVER_SDK_SOURCE_DIR}"
  "-DTEST_MODE=${TEST_MODE}"
)
if(DEFINED TEST_GENERATOR_PLATFORM AND NOT TEST_GENERATOR_PLATFORM STREQUAL "")
  list(APPEND _configure_command -A "${TEST_GENERATOR_PLATFORM}")
endif()
if(DEFINED TEST_GENERATOR_TOOLSET AND NOT TEST_GENERATOR_TOOLSET STREQUAL "")
  list(APPEND _configure_command -T "${TEST_GENERATOR_TOOLSET}")
endif()

execute_process(
  COMMAND ${_configure_command}
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)
if(TEST_MODE STREQUAL "default")
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR
      "Default solver bundle destination fixture failed:\n${_stdout}\n${_stderr}")
  endif()
elseif(_result EQUAL 0)
  message(FATAL_ERROR
    "Bare solver bundle DESTINATION unexpectedly configured successfully.")
elseif(NOT DEFINED EXPECTED_ERROR_PATTERN OR EXPECTED_ERROR_PATTERN STREQUAL "")
  message(FATAL_ERROR "EXPECTED_ERROR_PATTERN is required for invalid bundle modes.")
else()
  set(_diagnostic "${_stdout}\n${_stderr}")
  string(REGEX REPLACE "[ \t\r\n]+" " " _normalized_diagnostic "${_diagnostic}")
  if(NOT _normalized_diagnostic MATCHES "${EXPECTED_ERROR_PATTERN}")
    message(FATAL_ERROR
      "Bare solver bundle DESTINATION failed with an unexpected diagnostic.\n"
      "Expected pattern: ${EXPECTED_ERROR_PATTERN}\n"
      "Actual output:\n${_diagnostic}")
  endif()
endif()
