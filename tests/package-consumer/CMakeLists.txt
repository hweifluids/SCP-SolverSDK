cmake_minimum_required(VERSION 3.24)

project(SCPSolverSDKConsumer VERSION 0.1.0 LANGUAGES CXX)

find_package(SCPSolverSDK CONFIG REQUIRED)

include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverInfo.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverBundle.cmake")

add_executable(scp_sdk_package_consumer main.cpp)
target_link_libraries(scp_sdk_package_consumer PRIVATE SCP::SolverSDK)
streamcenterplus_configure_solver_info(
  scp_sdk_package_consumer "SDKSmoke" ON OFF "unstructuredmesh"
)
streamcenterplus_install_solver_bundle(
  scp_sdk_package_consumer DESTINATION "bin/unstructured"
)

