cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDK VERSION 0.1.0 LANGUAGES CXX)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SCP_SOLVER_SDK_DECLARED_VERSION LIMIT_COUNT 1)
if(NOT SCP_SOLVER_SDK_DECLARED_VERSION STREQUAL PROJECT_VERSION)
  message(FATAL_ERROR
    "SCP-SolverSDK VERSION does not match the CMake project version: "
    "${SCP_SOLVER_SDK_DECLARED_VERSION} != ${PROJECT_VERSION}")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
