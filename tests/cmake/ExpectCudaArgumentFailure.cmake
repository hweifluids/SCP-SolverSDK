cmake_minimum_required(VERSION 3.24)

foreach(_required IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    FIXTURE_SOURCE_DIR
    FIXTURE_BINARY_DIR
    TEST_GENERATOR
    TEST_ARGUMENT_FAILURE_MODE
    EXPECTED_ERROR_PATTERN)
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
  "-DSCP_SOLVER_SDK_SOURCE_DIR=${SCP_SOLVER_SDK_SOURCE_DIR}"
  "-DTEST_HELPER=add"
  "-DTEST_ARGUMENT_FAILURE_MODE=${TEST_ARGUMENT_FAILURE_MODE}"
  "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=2"
  "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2"
  "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
  "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
  "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
  "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
)
if(DEFINED TEST_GENERATOR_PLATFORM AND
   NOT TEST_GENERATOR_PLATFORM STREQUAL "")
  list(APPEND _configure_command -A "${TEST_GENERATOR_PLATFORM}")
endif()
if(DEFINED TEST_GENERATOR_TOOLSET AND
   NOT TEST_GENERATOR_TOOLSET STREQUAL "")
  list(APPEND _configure_command -T "${TEST_GENERATOR_TOOLSET}")
endif()

execute_process(
  COMMAND ${_configure_command}
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)
if(_result EQUAL 0)
  message(FATAL_ERROR
    "Invalid CUDA helper argument mode '${TEST_ARGUMENT_FAILURE_MODE}' "
    "unexpectedly configured successfully.")
endif()

set(_diagnostic "${_stdout}\n${_stderr}")
string(REGEX REPLACE "[ \t\r\n]+" " " _normalized_diagnostic "${_diagnostic}")
if(NOT _normalized_diagnostic MATCHES "${EXPECTED_ERROR_PATTERN}")
  message(FATAL_ERROR
    "Invalid CUDA helper arguments failed with an unexpected diagnostic.\n"
    "Expected pattern: ${EXPECTED_ERROR_PATTERN}\n"
    "Actual output:\n${_diagnostic}")
endif()

message(STATUS
  "Invalid CUDA helper argument mode '${TEST_ARGUMENT_FAILURE_MODE}' "
  "produced the expected diagnostic.")
