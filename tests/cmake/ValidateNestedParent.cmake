foreach(_required_variable IN ITEMS
    SCP_SOLVER_SDK_SOURCE_DIR
    FIXTURE_SOURCE_DIR
    FIXTURE_BINARY_ROOT
    TEST_GENERATOR
    TEST_CTEST_COMMAND)
  if(NOT DEFINED ${_required_variable} OR "${${_required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${_required_variable} is required.")
  endif()
endforeach()

function(_scp_solver_sdk_run_checked output_variable description)
  execute_process(
    COMMAND ${ARGN}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    ENCODING UTF-8
  )
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR
      "${description} failed with exit code ${_result}.\n"
      "stdout:\n${_stdout}\n"
      "stderr:\n${_stderr}")
  endif()
  set(${output_variable} "${_stdout}" PARENT_SCOPE)
endfunction()

set(_generator_arguments -G "${TEST_GENERATOR}")
if(DEFINED TEST_GENERATOR_PLATFORM AND NOT "${TEST_GENERATOR_PLATFORM}" STREQUAL "")
  list(APPEND _generator_arguments -A "${TEST_GENERATOR_PLATFORM}")
endif()
if(DEFINED TEST_GENERATOR_TOOLSET AND NOT "${TEST_GENERATOR_TOOLSET}" STREQUAL "")
  list(APPEND _generator_arguments -T "${TEST_GENERATOR_TOOLSET}")
endif()

get_filename_component(_fixture_binary_root "${FIXTURE_BINARY_ROOT}" ABSOLUTE)
if(_fixture_binary_root STREQUAL "" OR _fixture_binary_root STREQUAL "/")
  message(FATAL_ERROR "Refusing to use an unsafe nested-parent fixture root.")
endif()
set(_default_binary_dir "${_fixture_binary_root}/d")
set(_explicit_binary_dir "${_fixture_binary_root}/e")
file(REMOVE_RECURSE "${_default_binary_dir}" "${_explicit_binary_dir}")

_scp_solver_sdk_run_checked(
  _default_configure_output
  "Nested-parent default configure"
  "${CMAKE_COMMAND}"
    -S "${FIXTURE_SOURCE_DIR}"
    -B "${_default_binary_dir}"
    ${_generator_arguments}
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release"
    "-DSCP_SOLVER_SDK_SOURCE_DIR=${SCP_SOLVER_SDK_SOURCE_DIR}"
    "-DEXPECT_SDK_TESTS=OFF"
)
_scp_solver_sdk_run_checked(
  _default_build_output
  "Nested-parent default Release build"
  "${CMAKE_COMMAND}" --build "${_default_binary_dir}" --config Release --parallel
)
_scp_solver_sdk_run_checked(
  _default_tests_json
  "Nested-parent default CTest inventory"
  "${TEST_CTEST_COMMAND}" --test-dir "${_default_binary_dir}" -C Release
    --show-only=json-v1
)
string(JSON _default_test_count LENGTH "${_default_tests_json}" tests)
if(NOT _default_test_count EQUAL 1)
  message(FATAL_ERROR
    "A nested SDK leaked tests into its parent by default; expected only the parent sentinel, "
    "found ${_default_test_count}.\n${_default_tests_json}")
endif()
string(JSON _default_test_name GET "${_default_tests_json}" tests 0 name)
if(NOT _default_test_name STREQUAL "scp_solver_sdk_nested_parent_sentinel")
  message(FATAL_ERROR
    "Unexpected default nested-parent test inventory: ${_default_test_name}")
endif()

_scp_solver_sdk_run_checked(
  _explicit_configure_output
  "Nested-parent explicit configure"
  "${CMAKE_COMMAND}"
    -S "${FIXTURE_SOURCE_DIR}"
    -B "${_explicit_binary_dir}"
    ${_generator_arguments}
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release"
    "-DCMAKE_DISABLE_FIND_PACKAGE_VTK=ON"
    "-DSCP_SOLVER_SDK_SOURCE_DIR=${SCP_SOLVER_SDK_SOURCE_DIR}"
    "-DSCP_SOLVER_SDK_BUILD_TESTING=ON"
    "-DEXPECT_SDK_TESTS=ON"
)
_scp_solver_sdk_run_checked(
  _explicit_build_output
  "Nested-parent explicit Release build"
  "${CMAKE_COMMAND}" --build "${_explicit_binary_dir}" --config Release --parallel
)
_scp_solver_sdk_run_checked(
  _explicit_tests_json
  "Nested-parent explicit CTest inventory"
  "${TEST_CTEST_COMMAND}" --test-dir "${_explicit_binary_dir}" -C Release
    --show-only=json-v1
)
string(JSON _explicit_test_count LENGTH "${_explicit_tests_json}" tests)
set(_found_visualization_test OFF)
if(_explicit_test_count GREATER 0)
  math(EXPR _explicit_last_test "${_explicit_test_count} - 1")
  foreach(_test_index RANGE 0 ${_explicit_last_test})
    string(JSON _test_name GET "${_explicit_tests_json}" tests ${_test_index} name)
    if(_test_name STREQUAL "scp_solver_sdk_visualization_manifest")
      set(_found_visualization_test ON)
      break()
    endif()
  endforeach()
endif()
if(NOT _found_visualization_test)
  message(FATAL_ERROR
    "Explicit nested enablement did not register the SDK visualization test.\n"
    "${_explicit_tests_json}")
endif()
_scp_solver_sdk_run_checked(
  _explicit_test_output
  "Explicitly enabled nested SDK visualization test"
  "${TEST_CTEST_COMMAND}" --test-dir "${_explicit_binary_dir}" -C Release
    --output-on-failure -R "^scp_solver_sdk_visualization_manifest$"
)
