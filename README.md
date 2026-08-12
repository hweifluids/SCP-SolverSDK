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

## Visualization result manifest

`streamcenterplus/VisualizationManifest.h` defines the typed visualization
catalog embedded by `WriteOutputManifest` in
`streamcenterplus.output_manifest.v2`. A catalog identifies the canonical
solver family and run generation, declares fixed or numeric selector axes,
describes viewable quantities, and maps every actually published selector
tuple to a relative result path plus an optional VTKHDF step. Numeric axis
values are enumerated from produced results rather than inferred from deck
ranges. A variant may omit axes that do not apply to its branch; every selector
it does provide must name a declared axis and value. The catalog may publish
only the sparse tuples that actually exist. All manifest strings and relative
variant paths use UTF-8.

The existing `vtk_files` inventory remains in the v2 document for compatibility.
New consumers should prefer `visualization_catalog`; an empty catalog means that
the producer has not yet registered semantic result variants. Catalog paths are
validated to remain inside the output directory and the completed manifest is
published by atomic replacement, so a failed solve cannot expose a partially
written catalog.

## Requirements

- CMake 3.24 or newer.
- A C++17 compiler for consuming applications.
- Windows: Visual Studio 2022 C++ Build Tools with the v143 toolset and an x64 Windows SDK. The Windows installer and smoke consumer deliberately use this toolchain rather than compilers discovered from `PATH`.
- CUDA, MPI, VTK, HDF5, and other numerical dependencies are requirements of individual consumers, not of this SDK. `CpuCuda` consumers require CUDA Toolkit 13.2.x exactly.

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
find_package(SCPSolverSDK 0.2.0 EXACT CONFIG REQUIRED)
target_link_libraries(my_solver PRIVATE SCP::SolverSDK)

include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusOptionalCuda.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverInfo.cmake")
include("${SCP_SOLVER_SDK_CMAKE_DIR}/StreamcenterPlusSolverBundle.cmake")
```

Pass the SDK installation prefix through CMake's `CMAKE_PREFIX_PATH` or `<Package>_DIR` configure option. No environment variable is required.

## CPU/CUDA build modes

Including `StreamcenterPlusOptionalCuda.cmake` defines the cache setting `STREAMCENTERPLUS_BUILD_MODE`, whose supported values are `CpuOnly` (the default) and `CpuCuda`. `CpuOnly` never probes or enables the CUDA language. `CpuCuda` fails configuration unless both nvcc and CUDAToolkit are CUDA 13.2.x, then compiles a single hybrid solver target for every base compute capability reported by that toolchain: `sm_75`, `sm_80`, `sm_86`, `sm_87`, `sm_88`, `sm_89`, `sm_90`, `sm_100`, `sm_103`, `sm_110`, `sm_120`, and `sm_121`, plus a `compute_120` PTX fallback. The fully optimized compute-120 device IR feeds both native sm-120 and sm-121 assembly, and the retained PTX remains forward-compatible with newer devices. Runtime selection remains the solver configuration's `compute_backend=cpu|cuda` setting.

The global requests default to 12 architecture workers and two split-compile optimizer threads. Every CUDA target then applies a finite local safety cap: both caps default to 2 unless the module explicitly opts into a measured higher value. The effective value is the lesser of the request and cap; request value `0` means “use this target's cap”, not unbounded host concurrency. `streamcenterplus_add_optional_cuda` and `streamcenterplus_apply_cuda_release_codegen` accept the named one-value arguments `ARCHITECTURE_THREADS_CAP` and `SPLIT_COMPILE_THREADS_CAP`. Configuration reports the requested, capped, and effective values separately so a packaging log is auditable.

The supported split-compile benchmark sweep is selected explicitly at configure time with `-DSTREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS=2`, `=4`, or `=8`, but a target can use a candidate only when its local cap permits it. These entries are benchmark candidates, not a claim that a higher value is universally faster; the production request remains 2 until representative full-fat timing, peak-memory, generated-code, numerical-result, and runtime-performance measurements justify a change. None of these concurrency settings changes the published architecture matrix.

For a direct CMake invocation, the same two cache entries can be initialized
from the process-scoped `STREAMCENTERPLUS_CUDA_ARCHITECTURE_THREADS` and
`STREAMCENTERPLUS_CUDA_SPLIT_COMPILE_THREADS` environment variables. An explicit
`-D` value or existing CMake cache takes precedence over that initialization.
The repository component/release orchestrator has a different, intentional
contract: its command-line request is authoritative, so it refreshes matching
entries in existing component caches and also seeds clean caches through the
child-process environment. All sources are strictly limited to the advertised
choices; invalid or empty values stop configuration. The orchestrator restores
the caller's environment in `finally`, so its policy does not leak into
unrelated builds.

For compatibility with the published SolverSDK contract,
`streamcenterplus_add_optional_cuda` defaults to whole-program device
compilation. Streamcenter+ release solvers keep all device calls within one
CUDA translation unit and also pass `NO_SEPARABLE` explicitly. That preserves
whole-program device optimization, avoids an unnecessary device-link step, and
prevents a runtime-relevant code-generation mode from changing through a
helper default. A target with genuine cross-translation-unit device references
may opt in with `SEPARABLE` only after validating its device-link architecture
matrix, generated code, numerical results, and runtime performance.
`SEPARABLE` and `NO_SEPARABLE` cannot be combined. The conservative target cap
remains 2/2 unless a module opts into measured higher values.

CUDA helper control keywords (`SEPARABLE`, `NO_SEPARABLE`,
`ARCHITECTURE_THREADS_CAP`, and `SPLIT_COMPILE_THREADS_CAP`) must appear before
the multi-value `SOURCES` and `LIBRARIES` sections. This keeps unknown or
misspelled policy keywords fail-closed during `CpuOnly` configuration. The
formal release gate additionally parses all 11 packaged CUDA call sites and
requires this ordering, one CUDA translation unit, explicit `NO_SEPARABLE`, and
the approved target-cap mapping.

From the superproject root, reproducible full-fat compile sweeps are recorded
with:

```powershell
.\scripts\Benchmark-CudaCompile.ps1 `
  -Name SCP-FSLE `
  -ArchitectureThreads 12 `
  -SplitCompileThreads 2,4,8,8,4,2
```

The benchmark writes source/toolchain/cache fingerprints, process-tree-scoped
CUDA compiler memory, per-run artifacts, and an atomically updated
`results.json` under `.tests/cuda-compile-benchmark/`; failures are persisted
before the command exits nonzero.

## Windows CUDA compiler cache

The Windows Visual Studio 2022/CUDA 13.2 build path can place ccache in front of
the target-level `nvcc` invocation. This changes only how an object is obtained
during compilation. It does not change a solver algorithm, CUDA source,
architecture matrix, optimization flags, linked runtime, backend selection, or
runtime execution path. The patched cache-miss path invokes CUDA 13.2 with the
same ordered child `argv[]` elements supplied by MSBuild after removing only
ccache control arguments; it does not reconstruct or reorder the NVCC command.
A hit reuses an object previously produced for the same exact arguments,
verified source closure, toolchain and policy.

The repository policy is intentionally conservative:

- `direct_mode = false`, so every lookup still preprocesses the CUDA source and
  hashes transitively included headers before deciding whether an entry matches.
- `compiler_check = content`, so replacing a compiler at the same path does not
  preserve entries merely because timestamps look compatible.
- No `base_dir` path rewriting and no ccache `sloppiness` are enabled.
- The repository-local cache is limited to 64 GB and stored without compression
  to favor lookup speed on the build machine.

In addition to ccache's normal preprocessed-input checks, Streamcenter+ assigns
each build a content-derived namespace. Its v4 toolchain fingerprint covers the
verified patched ccache executable and tracked policy, CUDA compiler/headers,
the complete `nvvm` tree (including `libdevice.10.bc`), CUDA MSBuild
integration files, Visual Studio 2022 CUDA build customizations, the selected
x64 MSVC and Windows SDK toolchain, and hidden compiler inputs
`NVCC_PREPEND_FLAGS`, `NVCC_APPEND_FLAGS`, `CUDAFE_FLAGS`, `PTXAS_OPTIONS`,
`CL`, and `_CL_`. Each CUDA component also receives a deterministic source
closure manifest through `CCACHE_EXTRAFILES`; this covers project and generated
sources, SolverSDK, and installed dependency include roots that a single outer
NVCC preprocessing pass may not expose. A source, toolchain or policy change
therefore moves work into a different namespace instead of silently reusing an
older object.

For formal release children, the parent supplies both the v4
content-addressed toolchain manifest and its expected SHA-256. The child hashes
that manifest plus the current `nvcc`, `cl`, patched ccache, policy, and every
recorded compiler-affecting environment value instead of repeatedly hashing the
complete CUDA/MSVC/SDK header inventory. Manifest-without-hash is rejected;
standalone and legacy hash-only calls retain a full pre/post inventory. Each
actual CUDA component also receives its own closure-derived namespace and
`CCACHE_EXTRAFILES`; CPU-only components do not.

`StreamcenterPlusOptionalCuda.cmake` activates this path only when the component
or release orchestrator supplies
`STREAMCENTERPLUS_CUDA_COMPILER_CACHE_SHIM`. On Visual Studio generators the
helper redirects the CUDA target's `CudaToolkitNvccPath` to a verified
`nvcc.exe` masquerade; CUDA discovery and configuration still validate the real
CUDA 13.2 toolchain. The orchestrator also pins process-scoped `CUDACXX` and
rejects an existing CMake cache that names a different compiler, ensuring that
CMake discovery, the fingerprint, and ccache's real compiler all refer to the
same `nvcc.exe`. Direct CMake users who do not receive those process-scoped
environment variables continue to invoke `nvcc` normally.

Install the pinned Windows cache tool once from the superproject root:

```powershell
.\scripts\Install-CudaCompilerCache.ps1
```

The script downloads the locked official ccache 4.13.6 source archive, applies
the tracked Streamcenter+ NVCC exact-argv patch, builds Release, and records an
adjacent provenance/build manifest. The resolver requires the exact build
marker and verifies source, patch, patched-tree, manifest and executable
digests; an ordinary upstream 4.13.6 binary is not accepted. The developer
component orchestrator discovers the verified local build automatically:

```powershell
.\scripts\Install-Components.ps1 `
  -Scope solvers `
  -Name SCP-WaveletPOD `
  -BuildMode CpuCuda
```

`Install-Components.ps1` defaults to `-CudaCompilerCache Auto`. Use
`-CudaCompilerCache Required` when an unavailable or invalid cache must stop the
build, or `-CudaCompilerCache Off` for an intentional uncached diagnostic build.
`-CudaCompilerCachePath <path-to-ccache.exe>` selects an explicit executable;
an explicit missing, incompatible, or invalid path fails closed even in `Auto`
mode. `Off` cannot be combined with an explicit path.

The modified ccache is GPL-3.0-or-later and is a local build dependency only.
It is not linked into a solver and is not copied to module releases or the
customer installer. Its patch, provenance and license notices live under
`dependencies/ccache/` in the superproject.

After a cached component build, the orchestrator parses each formal generated
CUDA project and requires its target-level `CudaToolkitNvccPath` to equal the
verified shim. It also requires the exact generated CXX compiler to be the
fingerprinted x64 `cl.exe`, with the same Visual Studio instance, v143 toolset,
and Windows SDK version. It then audits the invocation stats log: direct mode,
unsupported invocations, compiler/cache errors, and inconsistent miss
accounting are rejected. A clean build must record at least one preprocessed
hit or miss for every formal `CudaCompile` item. CMake compiler probes,
dependency projects, and test trees are excluded from that target count.

A cache hit still runs CUDA preprocessing and dependency discovery; it avoids
the much more expensive device-code compilation but is not a zero-work build.
The cache lives under the superproject `.deps/compiler-cache/` tree, outside
component `.build/` directories. Consequently, deleting or clean-rebuilding a
component build tree does not discard valid cache entries.

## Solver identity types

`streamcenterplus_configure_solver_info` accepts:

- `structuredmesh`
- `unstructuredmesh`
- `tools`
- `developing`

Adding `unstructuredmesh` to the SDK contract does not make the current GUI execute an unstructured solver. The GUI remains responsible for selecting supported identity types and currently recognizes only `structuredmesh` and `tools`.
