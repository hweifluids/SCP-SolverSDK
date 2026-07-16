include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakeParseArguments)

# Keep solver bundles runnable on a clean Windows host.  Runtime dependency
# discovery deliberately excludes Windows system directories, so collect the
# redistributable MSVC/OpenMP DLLs from the active v143 toolchain separately
# and install them app-local for every solver destination.
if(MSVC)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)
  include(InstallRequiredSystemLibraries)
  set_property(GLOBAL PROPERTY STREAMCENTERPLUS_MSVC_RUNTIME_LIBS
    "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}"
  )
endif()

function(streamcenterplus_enable_cpu_parallel target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_enable_cpu_parallel: target '${target}' does not exist.")
  endif()

  find_package(OpenMP QUIET)
  if(OpenMP_CXX_FOUND)
    target_link_libraries("${target}" PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions("${target}" PUBLIC STREAMCENTERPLUS_CPU_OPENMP=1)
    message(STATUS "Enabled OpenMP CPU parallelism for ${target}.")
  else()
    message(STATUS "OpenMP was not found; ${target} will use serial CPU kernels.")
  endif()
endfunction()

function(streamcenterplus_install_solver_bundle target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "streamcenterplus_install_solver_bundle: target '${target}' does not exist.")
  endif()

  cmake_parse_arguments(ARG "" "DESTINATION" "" ${ARGN})
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "streamcenterplus_install_solver_bundle: unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT DEFINED ARG_DESTINATION OR ARG_DESTINATION STREQUAL "")
    set(ARG_DESTINATION ".")
  endif()
  if(IS_ABSOLUTE "${ARG_DESTINATION}" OR ARG_DESTINATION MATCHES "(^|[/\\\\])\\.\\.([/\\\\]|$)")
    message(FATAL_ERROR
      "streamcenterplus_install_solver_bundle: DESTINATION must be a relative install path without '..'.")
  endif()

  if(ARG_DESTINATION STREQUAL ".")
    set(bundle_library_destination "${CMAKE_INSTALL_LIBDIR}")
  else()
    set(bundle_library_destination "${ARG_DESTINATION}/${CMAKE_INSTALL_LIBDIR}")
  endif()

  string(MAKE_C_IDENTIFIER "${target}" runtime_dependency_set)
  set(runtime_dependency_set "${runtime_dependency_set}_runtime")

  if(UNIX AND NOT APPLE)
    set_target_properties("${target}" PROPERTIES
      INSTALL_RPATH "\$ORIGIN;\$ORIGIN/${CMAKE_INSTALL_LIBDIR}"
    )
  endif()

  set(runtime_dependency_args)
  set(runtime_search_dirs)

  if(DEFINED STREAMCENTERPLUS_PACKAGE_RUNTIME_DIRS AND NOT STREAMCENTERPLUS_PACKAGE_RUNTIME_DIRS STREQUAL "")
    list(APPEND runtime_search_dirs ${STREAMCENTERPLUS_PACKAGE_RUNTIME_DIRS})
  endif()
  if(WIN32 AND CMAKE_CXX_COMPILER)
    get_filename_component(compiler_runtime_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    list(APPEND runtime_search_dirs "${compiler_runtime_dir}")
  endif()
  if(runtime_search_dirs)
    list(REMOVE_DUPLICATES runtime_search_dirs)
    list(APPEND runtime_dependency_args DIRECTORIES ${runtime_search_dirs})
  endif()

  set(pre_exclude_regexes
    "api-ms-.*"
    "ext-ms-.*"
    "azureattest.*"
    "hvsifiletrust.*"
    "pdmutilities.*"
    "wpaxholder.*"
  )
  list(APPEND runtime_dependency_args PRE_EXCLUDE_REGEXES ${pre_exclude_regexes})

  if(WIN32)
    set(post_exclude_regexes
      ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/].*"
    )
  elseif(UNIX AND NOT APPLE)
    set(post_exclude_regexes
      ".*/ld-linux[^/]*\\.so[^/]*"
      ".*/libc\\.so(\\.[0-9]+)*"
      ".*/libdl\\.so(\\.[0-9]+)*"
      ".*/libm\\.so(\\.[0-9]+)*"
      ".*/libpthread\\.so(\\.[0-9]+)*"
      ".*/librt\\.so(\\.[0-9]+)*"
      ".*/libresolv\\.so(\\.[0-9]+)*"
      ".*/libnsl\\.so(\\.[0-9]+)*"
      ".*/libutil\\.so(\\.[0-9]+)*"
      ".*/libanl\\.so(\\.[0-9]+)*"
      ".*/libBrokenLocale\\.so(\\.[0-9]+)*"
    )
  else()
    set(post_exclude_regexes)
  endif()

  if(post_exclude_regexes)
    list(APPEND runtime_dependency_args POST_EXCLUDE_REGEXES ${post_exclude_regexes})
  endif()

  list(APPEND runtime_dependency_args
    RUNTIME DESTINATION "${ARG_DESTINATION}"
    LIBRARY DESTINATION "${bundle_library_destination}"
  )

  install(TARGETS "${target}"
    RUNTIME_DEPENDENCY_SET "${runtime_dependency_set}"
    RUNTIME DESTINATION "${ARG_DESTINATION}"
  )

  install(RUNTIME_DEPENDENCY_SET "${runtime_dependency_set}" ${runtime_dependency_args})

  if(WIN32)
    get_property(bundle_msvc_runtime_libs GLOBAL
      PROPERTY STREAMCENTERPLUS_MSVC_RUNTIME_LIBS
    )
    if(bundle_msvc_runtime_libs)
      install(PROGRAMS ${bundle_msvc_runtime_libs}
        DESTINATION "${ARG_DESTINATION}"
      )
    endif()
  endif()

  if(ARG_DESTINATION STREQUAL ".")
    set(bundle_license_destination "licenses")
  else()
    set(bundle_license_destination "${ARG_DESTINATION}/licenses")
  endif()

  if(STREAMCENTERPLUS_CUDA_AVAILABLE)
    set(cuda_toolkit_root_candidates "${CUDAToolkit_ROOT}")
    if(DEFINED CUDAToolkit_BIN_DIR AND NOT CUDAToolkit_BIN_DIR STREQUAL "")
      get_filename_component(cuda_root_from_bin "${CUDAToolkit_BIN_DIR}" DIRECTORY)
      list(APPEND cuda_toolkit_root_candidates "${cuda_root_from_bin}")
    endif()
    if(CMAKE_CUDA_COMPILER)
      get_filename_component(cuda_compiler_bin "${CMAKE_CUDA_COMPILER}" DIRECTORY)
      get_filename_component(cuda_root_from_compiler "${cuda_compiler_bin}" DIRECTORY)
      list(APPEND cuda_toolkit_root_candidates "${cuda_root_from_compiler}")
    endif()
    list(REMOVE_DUPLICATES cuda_toolkit_root_candidates)

    set(cuda_toolkit_license_root "")
    foreach(cuda_root_candidate IN LISTS cuda_toolkit_root_candidates)
      if(EXISTS "${cuda_root_candidate}/EULA.txt" OR
         EXISTS "${cuda_root_candidate}/LICENSE")
        set(cuda_toolkit_license_root "${cuda_root_candidate}")
        break()
      endif()
    endforeach()
    if(cuda_toolkit_license_root STREQUAL "")
      message(FATAL_ERROR
        "CUDA is enabled, but its EULA/LICENSE was not found below the CMake-discovered toolkit root."
      )
    endif()
    if(EXISTS "${cuda_toolkit_license_root}/EULA.txt")
      install(FILES "${cuda_toolkit_license_root}/EULA.txt"
        DESTINATION "${bundle_license_destination}"
        RENAME "NVIDIA-CUDA-EULA.txt"
      )
    endif()
    if(EXISTS "${cuda_toolkit_license_root}/LICENSE")
      install(FILES "${cuda_toolkit_license_root}/LICENSE"
        DESTINATION "${bundle_license_destination}"
        RENAME "NVIDIA-CUDA-LICENSE.txt"
      )
    endif()
  endif()

  string(MAKE_C_IDENTIFIER "${bundle_license_destination}" bundle_license_key)
  get_property(bundle_licenses_registered GLOBAL
    PROPERTY "STREAMCENTERPLUS_VCPKG_LICENSES_${bundle_license_key}"
  )
  if(NOT bundle_licenses_registered AND DEFINED VCPKG_INSTALLED_DIR)
    set(vcpkg_share_candidates)
    if(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
      list(APPEND vcpkg_share_candidates
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share"
      )
    endif()
    list(APPEND vcpkg_share_candidates "${VCPKG_INSTALLED_DIR}/share")

    set(vcpkg_share_root "")
    foreach(candidate IN LISTS vcpkg_share_candidates)
      if(IS_DIRECTORY "${candidate}")
        set(vcpkg_share_root "${candidate}")
        break()
      endif()
    endforeach()
    if(NOT vcpkg_share_root STREQUAL "")
      file(GLOB vcpkg_copyright_files LIST_DIRECTORIES FALSE
        "${vcpkg_share_root}/*/copyright"
      )
      foreach(copyright_file IN LISTS vcpkg_copyright_files)
        get_filename_component(port_directory "${copyright_file}" DIRECTORY)
        get_filename_component(port_name "${port_directory}" NAME)
        string(REGEX REPLACE "[^A-Za-z0-9._-]" "_" safe_port_name "${port_name}")
        install(FILES "${copyright_file}"
          DESTINATION "${bundle_license_destination}"
          RENAME "${safe_port_name}.txt"
        )
      endforeach()
      set_property(GLOBAL
        PROPERTY "STREAMCENTERPLUS_VCPKG_LICENSES_${bundle_license_key}" TRUE
      )
    endif()
  endif()
endfunction()
