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
  "-DSTREAMCENTERPLUS_BUILD_DATE=20260823"
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
  RESULT_VARIABLE _configure_result
  OUTPUT_VARIABLE _configure_stdout
  ERROR_VARIABLE _configure_stderr
)
if(TEST_MODE STREQUAL "matching" OR TEST_MODE STREQUAL "matching-cpu")
  set(_expected_cuda 1)
  if(TEST_MODE STREQUAL "matching-cpu")
    set(_expected_cuda 0)
  endif()
  if(NOT _configure_result EQUAL 0)
    message(FATAL_ERROR
      "Matching CUDA solver-info fixture failed to configure:\n"
      "${_configure_stdout}\n${_configure_stderr}")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${FIXTURE_BINARY_DIR}" --config Release --parallel
    RESULT_VARIABLE _build_result
    OUTPUT_VARIABLE _build_stdout
    ERROR_VARIABLE _build_stderr
  )
  if(NOT _build_result EQUAL 0)
    message(FATAL_ERROR
      "Matching CUDA solver-info fixture failed to build:\n"
      "${_build_stdout}\n${_build_stderr}")
  endif()
  set(_executable_suffix "${CMAKE_EXECUTABLE_SUFFIX}")
  if(WIN32)
    set(_executable_suffix ".exe")
  endif()
  set(_executable
    "${FIXTURE_BINARY_DIR}/Release/Test_solver_20260823${_executable_suffix}")
  if(NOT EXISTS "${_executable}")
    set(_executable
      "${FIXTURE_BINARY_DIR}/Test_solver_20260823${_executable_suffix}")
  endif()
  if(NOT EXISTS "${_executable}")
    message(FATAL_ERROR
      "Matching CUDA solver-info fixture did not produce its expected executable: "
      "${_executable}.")
  endif()
  execute_process(
    COMMAND "${_executable}" --solver-info
    RESULT_VARIABLE _solver_info_result
    OUTPUT_VARIABLE _solver_info
    ERROR_VARIABLE _solver_info_stderr
  )
  if(NOT _solver_info_result EQUAL 0 OR
     NOT _solver_info MATCHES "cuda=${_expected_cuda}")
    message(FATAL_ERROR
      "Matching CUDA solver-info fixture emitted invalid metadata:\n"
      "${_solver_info}\n${_solver_info_stderr}")
  endif()
elseif(_configure_result EQUAL 0)
  message(FATAL_ERROR
    "Invalid CUDA solver-info mode '${TEST_MODE}' unexpectedly configured successfully.")
elseif(NOT DEFINED EXPECTED_ERROR_PATTERN OR EXPECTED_ERROR_PATTERN STREQUAL "")
  message(FATAL_ERROR "EXPECTED_ERROR_PATTERN is required for invalid CUDA solver-info modes.")
else()
  set(_diagnostic "${_configure_stdout}\n${_configure_stderr}")
  string(REGEX REPLACE "[ \t\r\n]+" " " _normalized_diagnostic "${_diagnostic}")
  if(NOT _normalized_diagnostic MATCHES "${EXPECTED_ERROR_PATTERN}")
    message(FATAL_ERROR
      "Invalid CUDA solver-info mode '${TEST_MODE}' failed with an unexpected diagnostic.\n"
      "Expected pattern: ${EXPECTED_ERROR_PATTERN}\n"
      "Actual output:\n${_diagnostic}")
  endif()
endif()
