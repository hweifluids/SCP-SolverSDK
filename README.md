# SCP-SolverSDK

`SCP-SolverSDK` is the small, versioned compatibility layer shared by Streamcenter+ solver repositories. It is intentionally header-only and does not download or build third-party libraries.

## Contents

- `SCP::SolverSDK`: C++17 interface target exporting the SDK include directory.
- Solver identity support for `--solver-info`.
- CPU/CUDA backend parsing and optional-CUDA CMake helpers.
- VTK input-association, VTKHDF time-step, and output-manifest helpers.
- Runtime bundle installation helpers for self-contained solver releases.

The VTK-related headers are available through the SDK target, but a consumer that includes them must still locate and link the required VTK modules. The SDK itself has no VTK build-time dependency.

`examples/smoke` is a standalone installed-package consumer with no source-tree include fallback. Its Windows/Linux runners install the SDK, build a Release consumer, validate the `unstructuredmesh` solver signature, and execute the backend parser in one command.

## Requirements

- CMake 3.24 or newer.
- A C++17 compiler for consuming applications.
- Windows: Visual Studio 2022 C++ Build Tools with the v143 toolset and an x64 Windows SDK. The Windows installer and smoke consumer deliberately use this toolchain rather than compilers discovered from `PATH`.
- CUDA, MPI, VTK, HDF5, and other numerical dependencies are requirements of individual consumers, not of this SDK.

The SDK itself has no network dependency because it contains no third-party package download. A normal Streamcenter+ target may be online so solver repositories can fetch their separately pinned dependencies on first installation.

## One-command installation

Windows PowerShell:

```powershell
.\install.ps1
```

Linux:

```bash
bash ./install.sh
```

The installation is fixed inside this repository at `release/<platform>-<architecture>`. Build trees are fixed under `.build/`; both directories are ignored by Git. Installers expose no path parameters or environment-variable overrides. Use `-Clean` on Windows or `--clean` on Linux to remove only the SDK build tree before rebuilding.

## Use from the Streamcenter+ superproject

```cmake
add_subdirectory(modules/SCP-SolverSDK)

target_link_libraries(my_solver PRIVATE SCP::SolverSDK)
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverInfo.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverBundle.cmake")

streamcenterplus_configure_solver_info(
  my_solver "FTLE" ON OFF "structuredmesh"
)
streamcenterplus_install_solver_bundle(
  my_solver DESTINATION "structured"
)
```

## Use as an installed package

```cmake
find_package(SCPSolverSDK CONFIG REQUIRED)
target_link_libraries(my_solver PRIVATE SCP::SolverSDK)

include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusOptionalCuda.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverInfo.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverBundle.cmake")
```

Pass the SDK installation prefix through CMake's `CMAKE_PREFIX_PATH` or `<Package>_DIR` configure option. No environment variable is required.

## Solver identity types

`streamcenterplus_configure_solver_info` accepts:

- `structuredmesh`
- `unstructuredmesh`
- `tools`
- `developing`

Adding `unstructuredmesh` to the SDK contract does not make the current GUI execute an unstructured solver. The GUI remains responsible for selecting supported identity types and currently recognizes only `structuredmesh` and `tools`.
