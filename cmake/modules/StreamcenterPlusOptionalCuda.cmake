include_guard(DIRECTORY)

include(CheckLanguage)
include(CMakeParseArguments)

set(STREAMCENTERPLUS_CUDA_AVAILABLE OFF)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
  enable_language(CUDA)
  find_package(CUDAToolkit QUIET)
  if(CUDAToolkit_FOUND)
    set(STREAMCENTERPLUS_CUDA_AVAILABLE ON)

    set(_streamcenterplus_cuda_runtime_dirs ${STREAMCENTERPLUS_PACKAGE_RUNTIME_DIRS})
    foreach(_streamcenterplus_cuda_runtime_dir IN ITEMS
        "${CUDAToolkit_BIN_DIR}"
        "${CUDAToolkit_BIN_DIR}/x64"
        "${CUDAToolkit_ROOT}/bin"
        "${CUDAToolkit_ROOT}/bin/x64"
        "${CUDAToolkit_LIBRARY_DIR}")
      if(NOT _streamcenterplus_cuda_runtime_dir STREQUAL "" AND
         EXISTS "${_streamcenterplus_cuda_runtime_dir}")
        list(APPEND _streamcenterplus_cuda_runtime_dirs "${_streamcenterplus_cuda_runtime_dir}")
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _streamcenterplus_cuda_runtime_dirs)
    set(STREAMCENTERPLUS_PACKAGE_RUNTIME_DIRS "${_streamcenterplus_cuda_runtime_dirs}")
  endif()
endif()

set(STREAMCENTERPLUS_CUDA_AVAILABLE "${STREAMCENTERPLUS_CUDA_AVAILABLE}"
    CACHE INTERNAL "Whether the unified solver library has a usable CUDA backend" FORCE)

function(streamcenterplus_add_optional_cuda target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_add_optional_cuda: target '${target}' does not exist.")
  endif()

  cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

  if(STREAMCENTERPLUS_CUDA_AVAILABLE)
    target_sources("${target}" PRIVATE ${ARG_SOURCES})
    if(ARG_LIBRARIES)
      target_link_libraries("${target}" PUBLIC ${ARG_LIBRARIES})
    endif()
    target_compile_definitions("${target}" PUBLIC STREAMCENTERPLUS_HAS_CUDA=1)
    set_target_properties("${target}" PROPERTIES
      CUDA_STANDARD 17
      CUDA_STANDARD_REQUIRED ON
      CUDA_RUNTIME_LIBRARY Shared
      CUDA_SEPARABLE_COMPILATION ON
      CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
    if(CMAKE_CUDA_ARCHITECTURES)
      set_property(TARGET "${target}" PROPERTY CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
    else()
      set_property(TARGET "${target}" PROPERTY CUDA_ARCHITECTURES native)
    endif()
    target_compile_options("${target}" PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
    )
    message(STATUS "${target}: CUDA backend enabled (${CMAKE_CUDA_COMPILER}).")
  else()
    target_compile_definitions("${target}" PUBLIC STREAMCENTERPLUS_HAS_CUDA=0)
    message(STATUS "${target}: CUDA Toolkit/compiler unavailable; building the CPU backend only.")
  endif()
endfunction()

