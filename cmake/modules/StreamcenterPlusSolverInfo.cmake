include_guard(GLOBAL)

function(streamcenterplus_configure_solver_info target solver_name has_cpu has_cuda)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_configure_solver_info: target '${target}' does not exist.")
  endif()
  if(NOT TARGET SCP::SolverSDK)
    message(FATAL_ERROR
      "streamcenterplus_configure_solver_info requires the SCP::SolverSDK target. "
      "Add SCP-SolverSDK with add_subdirectory() or find_package(SCPSolverSDK CONFIG REQUIRED).")
  endif()

  if(DEFINED STREAMCENTERPLUS_BUILD_DATE AND NOT STREAMCENTERPLUS_BUILD_DATE STREQUAL "")
    set(_streamcenterplus_build_date "${STREAMCENTERPLUS_BUILD_DATE}")
  elseif(DEFINED ENV{STREAMCENTERPLUS_BUILD_DATE} AND NOT "$ENV{STREAMCENTERPLUS_BUILD_DATE}" STREQUAL "")
    set(_streamcenterplus_build_date "$ENV{STREAMCENTERPLUS_BUILD_DATE}")
  else()
    string(TIMESTAMP _streamcenterplus_build_date "%Y%m%d")
  endif()
  if(NOT _streamcenterplus_build_date MATCHES "^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$")
    message(FATAL_ERROR
      "STREAMCENTERPLUS_BUILD_DATE must use YYYYMMDD format; received '${_streamcenterplus_build_date}'.")
  endif()

  if(has_cpu)
    set(_streamcenterplus_has_cpu 1)
  else()
    set(_streamcenterplus_has_cpu 0)
  endif()
  if(has_cuda)
    set(_streamcenterplus_has_cuda 1)
  else()
    set(_streamcenterplus_has_cuda 0)
  endif()

  set(_streamcenterplus_solver_type "structuredmesh")
  if(ARGC GREATER 4)
    set(_streamcenterplus_solver_type "${ARGV4}")
  endif()
  if(NOT _streamcenterplus_solver_type MATCHES "^(structuredmesh|unstructuredmesh|tools|developing)$")
    message(FATAL_ERROR
      "Unsupported solver info type '${_streamcenterplus_solver_type}' for target '${target}'.")
  endif()

  set(_streamcenterplus_mesh_features "single_static")
  if(ARGC GREATER 5)
    set(_streamcenterplus_mesh_features "${ARGV5}")
  endif()
  if(NOT _streamcenterplus_mesh_features MATCHES "^[A-Za-z0-9_,-]+$")
    message(FATAL_ERROR
      "Solver mesh features for target '${target}' must be a comma-separated identifier list; received '${_streamcenterplus_mesh_features}'.")
  endif()

  target_link_libraries("${target}" PRIVATE SCP::SolverSDK)
  target_compile_definitions("${target}" PRIVATE
    STREAMCENTERPLUS_SOLVER_NAME="${solver_name}"
    STREAMCENTERPLUS_SOLVER_VERSION="${PROJECT_VERSION}"
    STREAMCENTERPLUS_SOLVER_TYPE="${_streamcenterplus_solver_type}"
    STREAMCENTERPLUS_BUILD_DATE="${_streamcenterplus_build_date}"
    STREAMCENTERPLUS_HAS_CPU=${_streamcenterplus_has_cpu}
    STREAMCENTERPLUS_HAS_CUDA=${_streamcenterplus_has_cuda}
    STREAMCENTERPLUS_MESH_FEATURES="${_streamcenterplus_mesh_features}"
  )
  set_target_properties("${target}" PROPERTIES
    OUTPUT_NAME "${solver_name}_solver_${_streamcenterplus_build_date}"
  )

  message(STATUS
    "${target}: solver info type=${_streamcenterplus_solver_type}, mesh_features=${_streamcenterplus_mesh_features}, date=${_streamcenterplus_build_date}, cpu=${_streamcenterplus_has_cpu}, cuda=${_streamcenterplus_has_cuda}.")
endfunction()
