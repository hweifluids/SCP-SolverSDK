include_guard(GLOBAL)

function(_streamcenterplus_validate_cuda_capability_dependency
    dependency expected_capability visited_targets)
  if(NOT TARGET "${dependency}")
    return()
  endif()

  get_property(_streamcenterplus_aliased_target
    TARGET "${dependency}" PROPERTY ALIASED_TARGET)
  if(NOT "${_streamcenterplus_aliased_target}" STREQUAL "")
    set(dependency "${_streamcenterplus_aliased_target}")
  endif()
  list(FIND visited_targets "${dependency}" _streamcenterplus_visited_index)
  if(NOT _streamcenterplus_visited_index EQUAL -1)
    return()
  endif()
  list(APPEND visited_targets "${dependency}")

  get_property(_streamcenterplus_interface_capability_is_set
    TARGET "${dependency}" PROPERTY
    INTERFACE_STREAMCENTERPLUS_CUDA_CAPABILITY SET)
  if(_streamcenterplus_interface_capability_is_set)
    get_target_property(_streamcenterplus_dependency_capability
      "${dependency}" INTERFACE_STREAMCENTERPLUS_CUDA_CAPABILITY)
    if(NOT "${_streamcenterplus_dependency_capability}" STREQUAL
           "${expected_capability}")
      message(FATAL_ERROR
        "Solver info CUDA capability '${expected_capability}' conflicts with "
        "the OptionalCuda capability '${_streamcenterplus_dependency_capability}' "
        "published by linked target '${dependency}'.")
    endif()
  endif()

  get_target_property(_streamcenterplus_interface_links
    "${dependency}" INTERFACE_LINK_LIBRARIES)
  if(_streamcenterplus_interface_links STREQUAL
     "_streamcenterplus_interface_links-NOTFOUND")
    return()
  endif()
  foreach(_streamcenterplus_interface_link IN LISTS
      _streamcenterplus_interface_links)
    if(_streamcenterplus_interface_link MATCHES "^\\$<LINK_ONLY:([^>]+)>$")
      set(_streamcenterplus_interface_link "${CMAKE_MATCH_1}")
    endif()
    _streamcenterplus_validate_cuda_capability_dependency(
      "${_streamcenterplus_interface_link}" "${expected_capability}"
      "${visited_targets}")
  endforeach()
endfunction()

function(_streamcenterplus_validate_linked_cuda_capability target expected_capability)
  if(NOT TARGET "${target}")
    return()
  endif()

  get_target_property(_streamcenterplus_linked_targets
    "${target}" LINK_LIBRARIES)
  if(_streamcenterplus_linked_targets STREQUAL
     "_streamcenterplus_linked_targets-NOTFOUND")
    return()
  endif()
  foreach(_streamcenterplus_linked_target IN LISTS
      _streamcenterplus_linked_targets)
    if(_streamcenterplus_linked_target MATCHES "^\\$<LINK_ONLY:([^>]+)>$")
      set(_streamcenterplus_linked_target "${CMAKE_MATCH_1}")
    endif()
    _streamcenterplus_validate_cuda_capability_dependency(
      "${_streamcenterplus_linked_target}" "${expected_capability}" "")
  endforeach()
endfunction()

function(streamcenterplus_configure_solver_info target solver_name has_cpu has_cuda)
  if(ARGC LESS 4 OR ARGC GREATER 6)
    message(FATAL_ERROR
      "streamcenterplus_configure_solver_info expects 4 to 6 arguments; received ${ARGC}.")
  endif()
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_configure_solver_info: target '${target}' does not exist.")
  endif()
  if(NOT solver_name MATCHES "^[A-Za-z0-9][A-Za-z0-9_.-]*$")
    message(FATAL_ERROR
      "Solver name for target '${target}' must be a non-empty single-line identifier; received '${solver_name}'.")
  endif()
  if(NOT DEFINED PROJECT_VERSION OR PROJECT_VERSION STREQUAL "")
    message(FATAL_ERROR
      "streamcenterplus_configure_solver_info requires a non-empty project VERSION.")
  endif()
  if(NOT TARGET SCP::SolverSDK)
    message(FATAL_ERROR
      "streamcenterplus_configure_solver_info requires the SCP::SolverSDK target. "
      "Add SCP-SolverSDK with add_subdirectory() or find_package(SCPSolverSDK CONFIG REQUIRED).")
  endif()

  foreach(_streamcenterplus_bool_name IN ITEMS has_cpu has_cuda)
    string(TOUPPER "${${_streamcenterplus_bool_name}}" _streamcenterplus_bool_value)
    if(NOT _streamcenterplus_bool_value MATCHES "^(ON|OFF|TRUE|FALSE|0|1)$")
      message(FATAL_ERROR
        "${_streamcenterplus_bool_name} for target '${target}' must be one of ON, OFF, TRUE, FALSE, 1, or 0; received '${${_streamcenterplus_bool_name}}'.")
    endif()
  endforeach()

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

  get_property(_streamcenterplus_cuda_capability_is_set
    TARGET "${target}" PROPERTY STREAMCENTERPLUS_CUDA_CAPABILITY SET)
  if(_streamcenterplus_cuda_capability_is_set)
    get_target_property(_streamcenterplus_existing_cuda_capability
      "${target}" STREAMCENTERPLUS_CUDA_CAPABILITY)
    if(NOT "${_streamcenterplus_existing_cuda_capability}" STREQUAL
           "${_streamcenterplus_has_cuda}")
      message(FATAL_ERROR
        "Solver info CUDA capability '${_streamcenterplus_has_cuda}' for target "
        "'${target}' conflicts with its existing SolverSDK CUDA capability "
        "'${_streamcenterplus_existing_cuda_capability}'.")
    endif()
  endif()
  # Check target-local capability immediately and inspect linked OptionalCuda
  # targets at directory end, after callers have added their implementation
  # libraries. This preserves the existing helper call order used by solvers.
  set_property(TARGET "${target}" PROPERTY
    STREAMCENTERPLUS_CUDA_CAPABILITY "${_streamcenterplus_has_cuda}")
  # DEFER evaluates ordinary variable references only at directory end, after
  # this function scope has gone away. Schedule bracket arguments through EVAL
  # so the target and expected value are captured now.
  cmake_language(EVAL CODE
    "cmake_language(DEFER CALL _streamcenterplus_validate_linked_cuda_capability "
    "[[${target}]] [[${_streamcenterplus_has_cuda}]])")

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
  if(NOT _streamcenterplus_mesh_features MATCHES
     "^[A-Za-z0-9_-]+(,[A-Za-z0-9_-]+)*$")
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
    STREAMCENTERPLUS_SOLVER_HAS_CUDA=${_streamcenterplus_has_cuda}
    STREAMCENTERPLUS_MESH_FEATURES="${_streamcenterplus_mesh_features}"
  )
  set_target_properties("${target}" PROPERTIES
    OUTPUT_NAME "${solver_name}_solver_${_streamcenterplus_build_date}"
  )

  message(STATUS
    "${target}: solver info type=${_streamcenterplus_solver_type}, mesh_features=${_streamcenterplus_mesh_features}, date=${_streamcenterplus_build_date}, cpu=${_streamcenterplus_has_cpu}, cuda=${_streamcenterplus_has_cuda}.")
endfunction()
