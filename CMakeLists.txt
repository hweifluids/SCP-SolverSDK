cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDK VERSION 0.2.0 LANGUAGES CXX)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SCP_SOLVER_SDK_DECLARED_VERSION LIMIT_COUNT 1)
if(NOT SCP_SOLVER_SDK_DECLARED_VERSION STREQUAL PROJECT_VERSION)
  message(FATAL_ERROR
    "SCP-SolverSDK VERSION does not match the CMake project version: "
    "${SCP_SOLVER_SDK_DECLARED_VERSION} != ${PROJECT_VERSION}")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

add_library(SCP_SolverSDK INTERFACE)
add_library(SCP::SolverSDK ALIAS SCP_SolverSDK)
set_target_properties(SCP_SolverSDK PROPERTIES EXPORT_NAME SolverSDK)

target_compile_features(SCP_SolverSDK INTERFACE cxx_std_17)
target_include_directories(SCP_SolverSDK INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set(SCP_SOLVER_SDK_CMAKE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  CACHE INTERNAL "SCP-SolverSDK CMake helper module directory" FORCE
)

install(TARGETS SCP_SolverSDK
  EXPORT SCPSolverSDKTargets
)
install(DIRECTORY include/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(DIRECTORY cmake/modules/
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SCPSolverSDK/modules"
  FILES_MATCHING PATTERN "*.cmake"
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SCPSolverSDKConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SCPSolverSDKConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SCPSolverSDK"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SCPSolverSDKConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(EXPORT SCPSolverSDKTargets
  FILE SCPSolverSDKTargets.cmake
  NAMESPACE SCP::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SCPSolverSDK"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/SCPSolverSDKConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/SCPSolverSDKConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SCPSolverSDK"
)

if(BUILD_TESTING)
  add_executable(SCPSolverSDKVisualizationManifestTests
    tests/VisualizationManifestTest.cpp
  )
  target_link_libraries(SCPSolverSDKVisualizationManifestTests PRIVATE
    SCP::SolverSDK
  )
  add_test(
    NAME scp_solver_sdk_visualization_manifest
    COMMAND SCPSolverSDKVisualizationManifestTests
  )

  set(_scp_solver_sdk_cuda_policy_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/ValidateOptionalCudaPolicy.cmake")

  add_test(
    NAME scp_solver_sdk_cuda_policy_defaults
    COMMAND "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DEXPECTED_SPLIT_COMPILE_THREADS=2"
      -P "${_scp_solver_sdk_cuda_policy_test}"
  )

  foreach(_scp_solver_sdk_split_compile_threads IN ITEMS 2 4 8)
    add_test(
      NAME
        "scp_solver_sdk_cuda_policy_split_${_scp_solver_sdk_split_compile_threads}"
      COMMAND "${CMAKE_COMMAND}"
        "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=${_scp_solver_sdk_split_compile_threads}"
        "-DEXPECTED_SPLIT_COMPILE_THREADS=${_scp_solver_sdk_split_compile_threads}"
        -P "${_scp_solver_sdk_cuda_policy_test}"
    )
  endforeach()

  add_test(
    NAME scp_solver_sdk_cuda_policy_environment_initializers
    COMMAND "${CMAKE_COMMAND}" -E env
      "STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=8"
      "STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=4"
      "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DEXPECTED_ARCHITECTURE_THREADS=8"
      "-DEXPECTED_SPLIT_COMPILE_THREADS=4"
      -P "${_scp_solver_sdk_cuda_policy_test}"
  )

  add_test(
    NAME scp_solver_sdk_cuda_policy_cache_precedes_environment
    COMMAND "${CMAKE_COMMAND}" -E env
      "STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=8"
      "STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=4"
      "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=2"
      "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
      "-DEXPECTED_ARCHITECTURE_THREADS=2"
      "-DEXPECTED_SPLIT_COMPILE_THREADS=1"
      -P "${_scp_solver_sdk_cuda_policy_test}"
  )

  add_test(
    NAME scp_solver_sdk_cuda_policy_normal_variable_initializers
    COMMAND "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DNORMAL_ARCHITECTURE_THREADS=8"
      "-DNORMAL_SPLIT_COMPILE_THREADS=4"
      "-DEXPECTED_ARCHITECTURE_THREADS=8"
      "-DEXPECTED_SPLIT_COMPILE_THREADS=4"
      -P "${_scp_solver_sdk_cuda_policy_test}"
  )

  set(_scp_solver_sdk_cuda_cap_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/ValidateCudaThreadCaps.cmake")
  function(_scp_solver_sdk_add_cuda_cap_test test_name)
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
        "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        ${ARGN}
        -P "${_scp_solver_sdk_cuda_cap_test}"
    )
  endfunction()

  _scp_solver_sdk_add_cuda_cap_test(
    scp_solver_sdk_cuda_cap_defaults
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_test(
    scp_solver_sdk_cuda_cap_serial_request
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=1"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_test(
    scp_solver_sdk_cuda_cap_zero_uses_target_cap
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=0"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=0"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_test(
    scp_solver_sdk_cuda_cap_fsle_profile
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=8"
    "-DCAP_ARCHITECTURE_THREADS=12"
    "-DCAP_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=12"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=12"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=8"
  )
  _scp_solver_sdk_add_cuda_cap_test(
    scp_solver_sdk_cuda_cap_waveletpod_profile
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=8"
    "-DCAP_ARCHITECTURE_THREADS=12"
    "-DCAP_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=12"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=12"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  set(_scp_solver_sdk_cuda_cap_failure_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/ExpectCudaCapFailure.cmake")
  add_test(
    NAME scp_solver_sdk_cuda_cap_invalid_architecture
    COMMAND "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DINVALID_CAP_ARGUMENT=CAP_ARCHITECTURE_THREADS"
      "-DINVALID_CAP_VALUE=3"
      "-DEXPECTED_ERROR_PATTERN=ARCHITECTURE_THREADS_CAP must be one of"
      -P "${_scp_solver_sdk_cuda_cap_failure_test}"
  )
  add_test(
    NAME scp_solver_sdk_cuda_cap_invalid_split
    COMMAND "${CMAKE_COMMAND}"
      "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DINVALID_CAP_ARGUMENT=CAP_SPLIT_COMPILE_THREADS"
      "-DINVALID_CAP_VALUE=3"
      "-DEXPECTED_ERROR_PATTERN=SPLIT_COMPILE_THREADS_CAP must be one of"
      -P "${_scp_solver_sdk_cuda_cap_failure_test}"
  )

  set(_scp_solver_sdk_cuda_cap_target_fixture
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/CudaCapTargetFixture")
  set(_scp_solver_sdk_cuda_cap_target_generator_arguments
    -G "${CMAKE_GENERATOR}")
  if(NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
    list(APPEND _scp_solver_sdk_cuda_cap_target_generator_arguments
      -A "${CMAKE_GENERATOR_PLATFORM}")
  endif()
  if(NOT "${CMAKE_GENERATOR_TOOLSET}" STREQUAL "")
    list(APPEND _scp_solver_sdk_cuda_cap_target_generator_arguments
      -T "${CMAKE_GENERATOR_TOOLSET}")
  endif()
  function(_scp_solver_sdk_add_cuda_cap_target_test test_name)
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
        -S "${_scp_solver_sdk_cuda_cap_target_fixture}"
        -B "${CMAKE_CURRENT_BINARY_DIR}/tests/${test_name}"
        ${_scp_solver_sdk_cuda_cap_target_generator_arguments}
        "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        ${ARGN}
    )
  endfunction()

  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_cap_target_apply_defaults
    "-DTEST_HELPER=apply"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_cap_target_add_serial
    "-DTEST_HELPER=add"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=1"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
    "-DCAP_ARCHITECTURE_THREADS=12"
    "-DCAP_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=12"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=8"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_cap_target_add_zero_uses_cap
    "-DTEST_HELPER=add"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=0"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=0"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_cap_target_add_fsle
    "-DTEST_HELPER=add"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=8"
    "-DCAP_ARCHITECTURE_THREADS=12"
    "-DCAP_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=12"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=8"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=12"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=8"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_cap_target_add_waveletpod
    "-DTEST_HELPER=add"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=12"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=8"
    "-DCAP_ARCHITECTURE_THREADS=12"
    "-DCAP_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=12"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=12"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_separable_default_off
    "-DTEST_HELPER=add"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=1"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
    "-DEXPECTED_SEPARABLE=OFF"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_separable_explicit_on
    "-DTEST_HELPER=add"
    "-DTEST_SEPARABLE_MODE=explicit-on"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=1"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
    "-DEXPECTED_SEPARABLE=ON"
  )
  _scp_solver_sdk_add_cuda_cap_target_test(
    scp_solver_sdk_cuda_separable_explicit_off
    "-DTEST_HELPER=add"
    "-DTEST_SEPARABLE_MODE=explicit-off"
    "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=1"
    "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=1"
    "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=1"
    "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
    "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
    "-DEXPECTED_SEPARABLE=OFF"
  )
  set(_scp_solver_sdk_cuda_argument_failure_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/ExpectCudaArgumentFailure.cmake")
  function(_scp_solver_sdk_add_cuda_argument_failure_test
      test_name failure_mode expected_error_pattern)
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
        "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DFIXTURE_SOURCE_DIR=${_scp_solver_sdk_cuda_cap_target_fixture}"
        "-DFIXTURE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/${test_name}"
        "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
        "-DTEST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DTEST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DTEST_ARGUMENT_FAILURE_MODE=${failure_mode}"
        "-DEXPECTED_ERROR_PATTERN=${expected_error_pattern}"
        -P "${_scp_solver_sdk_cuda_argument_failure_test}"
    )
  endfunction()
  _scp_solver_sdk_add_cuda_argument_failure_test(
    scp_solver_sdk_cuda_reject_unknown_argument
    unknown-before-sources
    "received unknown arguments: UNKNOWN_CUDA_POLICY")
  _scp_solver_sdk_add_cuda_argument_failure_test(
    scp_solver_sdk_cuda_reject_missing_cap_value
    missing-cap-value
    "requires values for: ARCHITECTURE_THREADS_CAP")
  _scp_solver_sdk_add_cuda_argument_failure_test(
    scp_solver_sdk_cuda_reject_conflicting_separable
    conflicting-separable
    "cannot use SEPARABLE and NO_SEPARABLE together")
  if(WIN32 AND CMAKE_GENERATOR MATCHES "^Visual Studio")
    _scp_solver_sdk_add_cuda_cap_target_test(
      scp_solver_sdk_cuda_cache_target_property
      "-DTEST_HELPER=apply"
      "-DTEST_CUDA_CACHE_MODE=valid"
      "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=2"
      "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2"
      "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
      "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
      "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
      "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
    )
    _scp_solver_sdk_add_cuda_cap_target_test(
      scp_solver_sdk_cuda_cache_target_property_add
      "-DTEST_HELPER=add"
      "-DTEST_CUDA_CACHE_MODE=valid"
      "-DSTREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS=2"
      "-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2"
      "-DEXPECTED_EFFECTIVE_ARCHITECTURE_THREADS=2"
      "-DEXPECTED_EFFECTIVE_SPLIT_COMPILE_THREADS=2"
      "-DEXPECTED_ARCHITECTURE_THREADS_CAP=2"
      "-DEXPECTED_SPLIT_COMPILE_THREADS_CAP=2"
    )

    set(_scp_solver_sdk_cuda_cache_failure_test
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/ExpectCudaCacheFailure.cmake")
    foreach(_scp_solver_sdk_cuda_cache_failure IN ITEMS wrong_name missing)
      if(_scp_solver_sdk_cuda_cache_failure STREQUAL "wrong_name")
        set(_scp_solver_sdk_cuda_cache_expected_error
          "must be named nvcc.exe")
      else()
        set(_scp_solver_sdk_cuda_cache_expected_error
          "must name an existing absolute executable")
      endif()
      add_test(
        NAME
          "scp_solver_sdk_cuda_cache_reject_${_scp_solver_sdk_cuda_cache_failure}"
        COMMAND "${CMAKE_COMMAND}"
          "-DSCP_SOLVER_SDK_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
          "-DFIXTURE_SOURCE_DIR=${_scp_solver_sdk_cuda_cap_target_fixture}"
          "-DFIXTURE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/scp_solver_sdk_cuda_cache_reject_${_scp_solver_sdk_cuda_cache_failure}"
          "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
          "-DTEST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
          "-DTEST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
          "-DTEST_CUDA_CACHE_MODE=${_scp_solver_sdk_cuda_cache_failure}"
          "-DEXPECTED_ERROR_PATTERN=${_scp_solver_sdk_cuda_cache_expected_error}"
          -P "${_scp_solver_sdk_cuda_cache_failure_test}"
      )
    endforeach()
  endif()
endif()
