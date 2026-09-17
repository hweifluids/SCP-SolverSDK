#pragma once

#include <streamcenterplus/HardwareAwareness.h>

#include <iostream>
#include <string>

#ifndef STREAMCENTERPLUS_SOLVER_NAME
#error "STREAMCENTERPLUS_SOLVER_NAME must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_SOLVER_VERSION
#error "STREAMCENTERPLUS_SOLVER_VERSION must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_SOLVER_TYPE
#error "STREAMCENTERPLUS_SOLVER_TYPE must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_BUILD_DATE
#error "STREAMCENTERPLUS_BUILD_DATE must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_HAS_CPU
#error "STREAMCENTERPLUS_HAS_CPU must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_HAS_CUDA
#error "STREAMCENTERPLUS_HAS_CUDA must be configured for every solver executable."
#endif

#ifndef STREAMCENTERPLUS_MESH_FEATURES
#define STREAMCENTERPLUS_MESH_FEATURES "single_static"
#endif

#ifndef STREAMCENTERPLUS_CLUSTER_PACKAGE_FORMATS
#define STREAMCENTERPLUS_CLUSTER_PACKAGE_FORMATS "scpjob.tar"
#endif

#ifndef STREAMCENTERPLUS_CLUSTER_CONTROL_FORMAT
#define STREAMCENTERPLUS_CLUSTER_CONTROL_FORMAT "streamcenterplus_deck_key_value"
#endif

#ifndef STREAMCENTERPLUS_CLUSTER_PARTITION_MODES
#define STREAMCENTERPLUS_CLUSTER_PARTITION_MODES "cluster_preflight,precomputed"
#endif

#ifndef STREAMCENTERPLUS_HARDWARE_AWARENESS
#define STREAMCENTERPLUS_HARDWARE_AWARENESS 1
#endif

#ifndef STREAMCENTERPLUS_CUDA_ARCHITECTURES
#define STREAMCENTERPLUS_CUDA_ARCHITECTURES ""
#endif

#ifndef STREAMCENTERPLUS_CUDA_ARCHITECTURE_POLICY
#define STREAMCENTERPLUS_CUDA_ARCHITECTURE_POLICY ""
#endif

namespace streamcenterplus {

inline bool HandleSolverInfoRequest(int argc, char* const* argv) {
    if (argc != 2 || argv == nullptr || argv[1] == nullptr ||
        std::string(argv[1]) != "--solver-info") {
        return false;
    }

    const HardwareAwareness hardware = DetectHardwareAwareness();
    std::cout << "streamcenterplus_solver_identity=1\n"
              << "product=Streamcenter+\n"
              << "type=" STREAMCENTERPLUS_SOLVER_TYPE "\n"
              << "solver=" STREAMCENTERPLUS_SOLVER_NAME "\n"
              << "version=" STREAMCENTERPLUS_SOLVER_VERSION "\n"
              << "developer=Huanxia Wei\n"
              << "organization=The University of Manchester\n"
              << "date=" STREAMCENTERPLUS_BUILD_DATE "\n"
              << "cpu=" << (STREAMCENTERPLUS_HAS_CPU ? 1 : 0) << "\n"
              << "cuda=" << (STREAMCENTERPLUS_HAS_CUDA ? 1 : 0) << "\n"
              // A comma-separated, additive capability list.  Consumers that
              // predate this field continue to treat an absent value as
              // single_static.
              << "mesh_features=" STREAMCENTERPLUS_MESH_FEATURES "\n"
              << "cluster_package_formats=" STREAMCENTERPLUS_CLUSTER_PACKAGE_FORMATS "\n"
              << "cluster_control_format=" STREAMCENTERPLUS_CLUSTER_CONTROL_FORMAT "\n"
              << "cluster_partition_modes=" STREAMCENTERPLUS_CLUSTER_PARTITION_MODES "\n"
              << "hardware_awareness=" << (STREAMCENTERPLUS_HARDWARE_AWARENESS ? 1 : 0) << "\n"
              << "cuda_architectures=" STREAMCENTERPLUS_CUDA_ARCHITECTURES "\n"
              << "cuda_architecture_policy=" STREAMCENTERPLUS_CUDA_ARCHITECTURE_POLICY "\n"
              << "runtime.scheduler=" << hardware.scheduler << "\n"
              << "runtime.scheduler_job_id=" << hardware.schedulerJobId << "\n"
              << "runtime.mpi_world_size=" << hardware.mpiWorldSize << "\n"
              << "runtime.mpi_rank=" << hardware.mpiRank << "\n"
              << "runtime.mpi_local_rank=" << hardware.mpiLocalRank << "\n"
              << "runtime.cuda_visible_devices=" << hardware.cudaVisibleDevicesRaw << "\n"
              << "runtime.preferred_cuda_ordinal=" << PreferredCudaDeviceOrdinal(hardware) << "\n";
    return true;
}

}  // namespace streamcenterplus
