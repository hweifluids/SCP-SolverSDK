cmake_minimum_required(VERSION 3.24)

foreach(_required IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    INVALID_CAP_ARGUMENT
    INVALID_CAP_VALUE
    EXPECTED_ERROR_PATTERN)
  if(NOT DEFINED "${_required}" OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required.")
  endif()
endforeach()

set(_validation_script
  "${SCP_SOLVER_SDK_SOURCE_DIR}/tests/cmake/ValidateCudaThreadCaps.cmake")
execute_process(
  COMMAND "${CMAKE_COMMAND}"
    "-DSCP_SOLVER_SDK_SOURCE_DIR=${SCP_SOLVER_SDK_SOURCE_DIR}"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2"
    "-D${INVALID_CAP_ARGUMENT}=${INVALID_CAP_VALUE}"
    -P "${_validation_script}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)
if(_result EQUAL 0)
  message(FATAL_ERROR
    "Invalid CUDA cap ${INVALID_CAP_ARGUMENT}=${INVALID_CAP_VALUE} "
    "unexpectedly succeeded.")
endif()

set(_diagnostic "${_stdout}\n${_stderr}")
if(NOT _diagnostic MATCHES "${EXPECTED_ERROR_PATTERN}")
  message(FATAL_ERROR
    "Invalid CUDA cap failed with an unexpected diagnostic.\n"
    "Expected pattern: ${EXPECTED_ERROR_PATTERN}\n"
    "Actual output:\n${_diagnostic}")
endif()

message(STATUS
  "Invalid CUDA cap produced the expected diagnostic: "
  "${INVALID_CAP_ARGUMENT}=${INVALID_CAP_VALUE}.")
