#include <streamcenterplus/ComputeBackend.h>
#include <streamcenterplus/SolverInfo.h>

#ifndef SCP_SOLVER_SDK_EXPECTED_CUDA
#error "SCP_SOLVER_SDK_EXPECTED_CUDA must be configured by the fixture."
#endif

int main(int argc, char** argv) {
    if (streamcenterplus::CudaBackendCompiled() != (SCP_SOLVER_SDK_EXPECTED_CUDA != 0)) {
        return 2;
    }
    return streamcenterplus::HandleSolverInfoRequest(argc, argv) ? 0 : 1;
}
