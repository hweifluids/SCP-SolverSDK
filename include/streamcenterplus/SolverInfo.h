#pragma once

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

namespace streamcenterplus {

inline bool HandleSolverInfoRequest(int argc, char* const* argv) {
    if (argc != 2 || argv == nullptr || argv[1] == nullptr ||
        std::string(argv[1]) != "--solver-info") {
        return false;
    }

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
              << "mesh_features=" STREAMCENTERPLUS_MESH_FEATURES "\n";
    return true;
}

}  // namespace streamcenterplus
