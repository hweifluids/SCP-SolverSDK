include_guard(DIRECTORY)

include(CheckLanguage)
include(CMakeParseArguments)

set(STREAMCENTERPLUS_BUILD_MODE "CpuOnly" CACHE STRING
    "Streamcenter+ solver build mode: CpuOnly or CpuCuda")
set_property(CACHE STREAMCENTERPLUS_BUILD_MODE PROPERTY STRINGS CpuOnly CpuCuda)
if(NOT STREAMCENTERPLUS_BUILD_MODE STREQUAL "CpuOnly" AND
   NOT STREAMCENTERPLUS_BUILD_MODE STREQUAL "CpuCuda")
  message(FATAL_ERROR
    "STREAMCENTERPLUS_BUILD_MODE must be CpuOnly or CpuCuda; got "
    "'${STREAMCENTERPLUS_BUILD_MODE}'.")
endif()

# CUDA 13.2 is the single release toolchain. Embed every base compute capability
# reported by that toolchain so workstation, data-centre, and embedded NVIDIA
# systems do not depend on forward JIT. Compile the newest native sm_120 and
# sm_121 images from one fully optimized compute_120 PTX front end, then retain
# that PTX as the forward-compatibility fallback. This avoids a redundant
# architecture-specific front end while preserving native code for every known
# device and JIT support for newer devices. Never use `native`: it emits no PTX
# and ties a release artifact to the GPU visible on the build host.
set(STREAMCENTERPLUS_CUDA_ARCHITECTURES
    "75-real;80-real;86-real;87-real;88-real;89-real;90-real;100-real;103-real;110-real;120-real;121-real;120-virtual"
    CACHE INTERNAL
    "CUDA 13.2 full release fat-binary architecture matrix"
    FORCE)
set(STREAMCENTERPLUS_CMAKE_CUDA_ARCHITECTURES
    "75-real;80-real;86-real;87-real;88-real;89-real;90-real;100-real;103-real;110-real"
    CACHE INTERNAL
    "Architectures emitted through CMake's one-compute-IR-per-SM mapping"
    FORCE)
set(STREAMCENTERPLUS_CUDA_COMBINED_BLACKWELL_CODEGEN
    "--generate-code=arch=compute_120,code=[sm_120,sm_121,compute_120]"
    CACHE INTERNAL
    "CUDA 13.2 combined sm_120, sm_121, and forward-PTX code generation"
    FORCE)
set(_streamcenterplus_cuda_split_compile_threads_default "2")
get_property(_streamcenterplus_cuda_split_cache_type_set
  CACHE STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS PROPERTY TYPE SET)
if(NOT _streamcenterplus_cuda_split_cache_type_set)
  if(DEFINED STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS)
    set(_streamcenterplus_cuda_split_compile_threads_default
        "${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}")
  elseif(DEFINED ENV{STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS})
    set(_streamcenterplus_cuda_split_compile_threads_default
        "$ENV{STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}")
  endif()
  set(STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS
      "${_streamcenterplus_cuda_split_compile_threads_default}" CACHE STRING
      "Requested CUDA compiler optimization threads (0 requests the target cap)")
else()
  get_property(_streamcenterplus_cuda_split_cache_type
    CACHE STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS PROPERTY TYPE)
  if(_streamcenterplus_cuda_split_cache_type STREQUAL "UNINITIALIZED")
    set(STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS
        "${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}" CACHE STRING
        "Requested CUDA compiler optimization threads (0 requests the target cap)"
        FORCE)
  endif()
endif()
unset(_streamcenterplus_cuda_split_compile_threads_default)
unset(_streamcenterplus_cuda_split_cache_type)
unset(_streamcenterplus_cuda_split_cache_type_set)
set_property(CACHE STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS PROPERTY STRINGS
    "1" "2" "4" "8" "0")
set(_streamcenterplus_cuda_split_compile_thread_choices 0 1 2 4 8)
if(NOT STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS IN_LIST
       _streamcenterplus_cuda_split_compile_thread_choices)
  message(FATAL_ERROR
    "STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS must be one of 0, 1, 2, 4, or 8; got "
    "'${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}'.")
endif()
unset(_streamcenterplus_cuda_split_compile_thread_choices)
set(_streamcenterplus_cuda_architecture_threads_default "12")
get_property(_streamcenterplus_cuda_architecture_cache_type_set
  CACHE STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS PROPERTY TYPE SET)
if(NOT _streamcenterplus_cuda_architecture_cache_type_set)
  if(DEFINED STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS)
    set(_streamcenterplus_cuda_architecture_threads_default
        "${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}")
  elseif(DEFINED ENV{STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS})
    set(_streamcenterplus_cuda_architecture_threads_default
        "$ENV{STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}")
  endif()
  set(STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS
      "${_streamcenterplus_cuda_architecture_threads_default}" CACHE STRING
      "Requested parallel CUDA architecture compilations (0 requests the target cap)")
else()
  get_property(_streamcenterplus_cuda_architecture_cache_type
    CACHE STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS PROPERTY TYPE)
  if(_streamcenterplus_cuda_architecture_cache_type STREQUAL "UNINITIALIZED")
    set(STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS
        "${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}" CACHE STRING
        "Requested parallel CUDA architecture compilations (0 requests the target cap)"
        FORCE)
  endif()
endif()
unset(_streamcenterplus_cuda_architecture_threads_default)
unset(_streamcenterplus_cuda_architecture_cache_type)
unset(_streamcenterplus_cuda_architecture_cache_type_set)
set_property(CACHE STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS PROPERTY STRINGS
    "1" "2" "4" "8" "12" "0")
set(_streamcenterplus_cuda_architecture_thread_choices 0 1 2 4 8 12)
if(NOT STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS IN_LIST
       _streamcenterplus_cuda_architecture_thread_choices)
  message(FATAL_ERROR
    "STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS must be one of 0, 1, 2, 4, 8, or 12; got "
    "'${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}'.")
endif()
unset(_streamcenterplus_cuda_architecture_thread_choices)

set(STREAMCENTERPLUS_CUDA_AVAILABLE OFF)
if(STREAMCENTERPLUS_BUILD_MODE STREQUAL "CpuCuda")
  set(CMAKE_CUDA_ARCHITECTURES "${STREAMCENTERPLUS_CMAKE_CUDA_ARCHITECTURES}"
      CACHE STRING "CUDA architectures selected by Streamcenter+ release policy" FORCE)

  check_language(CUDA)
  if(NOT CMAKE_CUDA_COMPILER)
    message(FATAL_ERROR
      "CpuCuda mode requires the CUDA 13.2 compiler, but CMake could not find nvcc. "
      "Install CUDA Toolkit 13.2 or configure with STREAMCENTERPLUS_BUILD_MODE=CpuOnly.")
  endif()

  enable_language(CUDA)
  find_package(CUDAToolkit 13.2 REQUIRED)
  if(CUDAToolkit_VERSION VERSION_LESS "13.2" OR
     NOT CUDAToolkit_VERSION VERSION_LESS "13.3" OR
     CMAKE_CUDA_COMPILER_VERSION VERSION_LESS "13.2" OR
     NOT CMAKE_CUDA_COMPILER_VERSION VERSION_LESS "13.3")
    message(FATAL_ERROR
      "CpuCuda release builds require CUDA Toolkit/compiler 13.2.x exactly; "
      "found toolkit ${CUDAToolkit_VERSION} and compiler ${CMAKE_CUDA_COMPILER_VERSION}.")
  endif()

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

set(STREAMCENTERPLUS_CUDA_AVAILABLE "${STREAMCENTERPLUS_CUDA_AVAILABLE}"
    CACHE INTERNAL "Whether the unified solver library has a usable CUDA backend" FORCE)

function(_streamcenterplus_resolve_cuda_parallelism
    output_architecture_threads
    output_split_compile_threads
    output_architecture_threads_cap
    output_split_compile_threads_cap)
  cmake_parse_arguments(PARSE_ARGV 4 ARG ""
    "ARCHITECTURE_THREADS_CAP;SPLIT_COMPILE_THREADS_CAP" "")
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "_streamcenterplus_resolve_cuda_parallelism received unknown arguments: "
      "${ARG_UNPARSED_ARGUMENTS}.")
  endif()
  if(ARG_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR
      "_streamcenterplus_resolve_cuda_parallelism requires values for: "
      "${ARG_KEYWORDS_MISSING_VALUES}.")
  endif()

  if("${ARG_ARCHITECTURE_THREADS_CAP}" STREQUAL "")
    set(ARG_ARCHITECTURE_THREADS_CAP "2")
  endif()
  if("${ARG_SPLIT_COMPILE_THREADS_CAP}" STREQUAL "")
    set(ARG_SPLIT_COMPILE_THREADS_CAP "2")
  endif()

  set(_streamcenterplus_architecture_cap_choices 1 2 4 8 12)
  if(NOT ARG_ARCHITECTURE_THREADS_CAP IN_LIST
         _streamcenterplus_architecture_cap_choices)
    message(FATAL_ERROR
      "ARCHITECTURE_THREADS_CAP must be one of 1, 2, 4, 8, or 12; got "
      "'${ARG_ARCHITECTURE_THREADS_CAP}'.")
  endif()
  set(_streamcenterplus_split_compile_cap_choices 1 2 4 8)
  if(NOT ARG_SPLIT_COMPILE_THREADS_CAP IN_LIST
         _streamcenterplus_split_compile_cap_choices)
    message(FATAL_ERROR
      "SPLIT_COMPILE_THREADS_CAP must be one of 1, 2, 4, or 8; got "
      "'${ARG_SPLIT_COMPILE_THREADS_CAP}'.")
  endif()

  if(STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS STREQUAL "0" OR
     STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS GREATER
       ARG_ARCHITECTURE_THREADS_CAP)
    set(_streamcenterplus_effective_architecture_threads
        "${ARG_ARCHITECTURE_THREADS_CAP}")
  else()
    set(_streamcenterplus_effective_architecture_threads
        "${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}")
  endif()
  if(STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS STREQUAL "0" OR
     STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS GREATER
       ARG_SPLIT_COMPILE_THREADS_CAP)
    set(_streamcenterplus_effective_split_compile_threads
        "${ARG_SPLIT_COMPILE_THREADS_CAP}")
  else()
    set(_streamcenterplus_effective_split_compile_threads
        "${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}")
  endif()

  set("${output_architecture_threads}"
      "${_streamcenterplus_effective_architecture_threads}" PARENT_SCOPE)
  set("${output_split_compile_threads}"
      "${_streamcenterplus_effective_split_compile_threads}" PARENT_SCOPE)
  set("${output_architecture_threads_cap}"
      "${ARG_ARCHITECTURE_THREADS_CAP}" PARENT_SCOPE)
  set("${output_split_compile_threads_cap}"
      "${ARG_SPLIT_COMPILE_THREADS_CAP}" PARENT_SCOPE)
endfunction()

function(_streamcenterplus_apply_cuda_compiler_cache target)
  if(NOT DEFINED ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM} OR
     "$ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM}" STREQUAL "")
    set_target_properties("${target}" PROPERTIES
      STREAMCENTERPLUS_CUDA_COMPILER_CACHE_ENABLED OFF
    )
    return()
  endif()

  if(NOT WIN32 OR
     NOT CMAKE_GENERATOR MATCHES "^Visual Studio [0-9]+ [0-9]+$")
    message(FATAL_ERROR
      "STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM is supported only by the "
      "Windows Visual Studio CUDA build path; generator='${CMAKE_GENERATOR}'.")
  endif()

  file(TO_CMAKE_PATH
    "$ENV{STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM}"
    _streamcenterplus_cuda_cache_shim)
  if(NOT IS_ABSOLUTE "${_streamcenterplus_cuda_cache_shim}" OR
     NOT EXISTS "${_streamcenterplus_cuda_cache_shim}" OR
     IS_DIRECTORY "${_streamcenterplus_cuda_cache_shim}")
    message(FATAL_ERROR
      "STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM must name an existing "
      "absolute executable: '${_streamcenterplus_cuda_cache_shim}'.")
  endif()
  get_filename_component(
    _streamcenterplus_cuda_cache_shim_name
    "${_streamcenterplus_cuda_cache_shim}"
    NAME)
  string(TOLOWER
    "${_streamcenterplus_cuda_cache_shim_name}"
    _streamcenterplus_cuda_cache_shim_name_lower)
  if(NOT _streamcenterplus_cuda_cache_shim_name_lower STREQUAL "nvcc.exe")
    message(FATAL_ERROR
      "The Visual Studio CUDA compiler-cache shim must be named nvcc.exe; "
      "got '${_streamcenterplus_cuda_cache_shim_name}'.")
  endif()

  # Visual Studio generators do not honor CMAKE_CUDA_COMPILER_LAUNCHER.
  # NVIDIA's CUDA MSBuild integration does honor the global NvccPath property,
  # so route only this target's nvcc custom build through the verified
  # masquerade. The cache miss path uses the same effective compiler options.
  set_property(
    TARGET "${target}"
    PROPERTY VS_GLOBAL_CudaToolkitNvccPath
      "${_streamcenterplus_cuda_cache_shim}")
  set_target_properties("${target}" PROPERTIES
    STREAMCENTERPLUS_CUDA_COMPILER_CACHE_ENABLED ON
    STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM
      "${_streamcenterplus_cuda_cache_shim}"
  )
  message(STATUS
    "${target}: CUDA compiler cache enabled through "
    "${_streamcenterplus_cuda_cache_shim}.")
endfunction()

function(streamcenterplus_apply_cuda_release_codegen target)
  cmake_parse_arguments(PARSE_ARGV 1 ARG ""
    "ARCHITECTURE_THREADS_CAP;SPLIT_COMPILE_THREADS_CAP" "")
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "streamcenterplus_apply_cuda_release_codegen received unknown arguments: "
      "${ARG_UNPARSED_ARGUMENTS}.")
  endif()
  if(ARG_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR
      "streamcenterplus_apply_cuda_release_codegen requires values for: "
      "${ARG_KEYWORDS_MISSING_VALUES}.")
  endif()

  if(NOT TARGET "${target}")
    message(FATAL_ERROR
      "streamcenterplus_apply_cuda_release_codegen: target '${target}' does not exist.")
  endif()
  if(NOT STREAMCENTERPLUS_CUDA_AVAILABLE)
    message(FATAL_ERROR
      "streamcenterplus_apply_cuda_release_codegen requires an enabled CUDA 13.2 backend.")
  endif()

  set(_streamcenterplus_parallel_cap_arguments)
  if(NOT "${ARG_ARCHITECTURE_THREADS_CAP}" STREQUAL "")
    list(APPEND _streamcenterplus_parallel_cap_arguments
      ARCHITECTURE_THREADS_CAP "${ARG_ARCHITECTURE_THREADS_CAP}")
  endif()
  if(NOT "${ARG_SPLIT_COMPILE_THREADS_CAP}" STREQUAL "")
    list(APPEND _streamcenterplus_parallel_cap_arguments
      SPLIT_COMPILE_THREADS_CAP "${ARG_SPLIT_COMPILE_THREADS_CAP}")
  endif()
  _streamcenterplus_resolve_cuda_parallelism(
    _streamcenterplus_effective_architecture_threads
    _streamcenterplus_effective_split_compile_threads
    _streamcenterplus_architecture_threads_cap
    _streamcenterplus_split_compile_threads_cap
    ${_streamcenterplus_parallel_cap_arguments})

  # CMake maps `121-real` to a separate compute_121 front-end invocation. CUDA
  # 13.2 cannot compile the largest Streamcenter+ translation units through that
  # front end. Emit the older architectures through CMake, then use one explicit
  # compute_120 invocation for native sm_120, native sm_121, and retained PTX.
  # This is also faster than compiling the same fully optimized device IR three
  # times and preserves the exact public fat-binary contract above.
  set_property(TARGET "${target}" PROPERTY CUDA_ARCHITECTURES
    "${STREAMCENTERPLUS_CMAKE_CUDA_ARCHITECTURES}")
  target_compile_options("${target}" PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:${STREAMCENTERPLUS_CUDA_COMBINED_BLACKWELL_CODEGEN}>
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
  )
  if(NOT _streamcenterplus_effective_split_compile_threads STREQUAL "1")
    target_compile_options("${target}" PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:--split-compile=${_streamcenterplus_effective_split_compile_threads}>
    )
  endif()
  if(NOT _streamcenterplus_effective_architecture_threads STREQUAL "1")
    target_compile_options("${target}" PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:--threads=${_streamcenterplus_effective_architecture_threads}>
    )
  endif()
  set_target_properties("${target}" PROPERTIES
    STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS_CAP
      "${_streamcenterplus_architecture_threads_cap}"
    STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS_CAP
      "${_streamcenterplus_split_compile_threads_cap}"
    STREAMCENTERPLUS_CUDA_EFFECTIVE_ARCHITECTURE_THREADS
      "${_streamcenterplus_effective_architecture_threads}"
    STREAMCENTERPLUS_CUDA_EFFECTIVE_SPLIT_COMPILE_THREADS
      "${_streamcenterplus_effective_split_compile_threads}"
  )
  _streamcenterplus_apply_cuda_compiler_cache("${target}")
  message(STATUS
    "${target}: CUDA parallelism requested "
    "architecture=${STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS}, "
    "split=${STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS}; "
    "cap architecture=${_streamcenterplus_architecture_threads_cap}, "
    "split=${_streamcenterplus_split_compile_threads_cap}; "
    "effective architecture=${_streamcenterplus_effective_architecture_threads}, "
    "split=${_streamcenterplus_effective_split_compile_threads}.")
endfunction()

function(streamcenterplus_add_optional_cuda target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_add_optional_cuda: target '${target}' does not exist.")
  endif()

  cmake_parse_arguments(PARSE_ARGV 1 ARG "SEPARABLE;NO_SEPARABLE"
    "ARCHITECTURE_THREADS_CAP;SPLIT_COMPILE_THREADS_CAP"
    "SOURCES;LIBRARIES")
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "streamcenterplus_add_optional_cuda received unknown arguments: "
      "${ARG_UNPARSED_ARGUMENTS}.")
  endif()
  if(ARG_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR
      "streamcenterplus_add_optional_cuda requires values for: "
      "${ARG_KEYWORDS_MISSING_VALUES}.")
  endif()
  if(ARG_SEPARABLE AND ARG_NO_SEPARABLE)
    message(FATAL_ERROR
      "streamcenterplus_add_optional_cuda cannot use SEPARABLE and "
      "NO_SEPARABLE together.")
  endif()

  # Preserve the published SolverSDK contract: CUDA targets use whole-program
  # device compilation unless a target explicitly opts into relocatable device
  # code. Formal release solvers spell NO_SEPARABLE explicitly so this runtime-
  # relevant code-generation mode cannot change through a helper default.
  set(_streamcenterplus_cuda_separable OFF)
  if(ARG_SEPARABLE)
    set(_streamcenterplus_cuda_separable ON)
  endif()

  set(_streamcenterplus_parallel_cap_arguments)
  if(NOT "${ARG_ARCHITECTURE_THREADS_CAP}" STREQUAL "")
    list(APPEND _streamcenterplus_parallel_cap_arguments
      ARCHITECTURE_THREADS_CAP "${ARG_ARCHITECTURE_THREADS_CAP}")
  endif()
  if(NOT "${ARG_SPLIT_COMPILE_THREADS_CAP}" STREQUAL "")
    list(APPEND _streamcenterplus_parallel_cap_arguments
      SPLIT_COMPILE_THREADS_CAP "${ARG_SPLIT_COMPILE_THREADS_CAP}")
  endif()
  # Validate target-local caps even in CpuOnly mode so lightweight configure
  # jobs catch misspelled or unsafe module policy before a release build.
  _streamcenterplus_resolve_cuda_parallelism(
    _streamcenterplus_unused_effective_architecture_threads
    _streamcenterplus_unused_effective_split_compile_threads
    _streamcenterplus_unused_architecture_threads_cap
    _streamcenterplus_unused_split_compile_threads_cap
    ${_streamcenterplus_parallel_cap_arguments})

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
      CUDA_SEPARABLE_COMPILATION "${_streamcenterplus_cuda_separable}"
      CUDA_RESOLVE_DEVICE_SYMBOLS "${_streamcenterplus_cuda_separable}"
    )
    streamcenterplus_apply_cuda_release_codegen(
      "${target}" ${_streamcenterplus_parallel_cap_arguments})
    message(STATUS
      "${target}: CUDA 13.2 backend enabled (${CMAKE_CUDA_COMPILER}); "
      "fat binary=${STREAMCENTERPLUS_CUDA_ARCHITECTURES}.")
  else()
    target_compile_definitions("${target}" PUBLIC STREAMCENTERPLUS_HAS_CUDA=0)
    message(STATUS "${target}: explicit CpuOnly build; CUDA sources are excluded.")
  endif()
endfunction()
