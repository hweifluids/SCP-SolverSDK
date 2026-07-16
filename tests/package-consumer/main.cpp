#include <streamcenterplus/ComputeBackend.h>
#include <streamcenterplus/SolverInfo.h>

int main(int argc, char** argv) {
    if (streamcenterplus::HandleSolverInfoRequest(argc, argv)) {
        return 0;
    }
    return streamcenterplus::ParseComputeBackend("cpu") == streamcenterplus::ComputeBackend::Cpu ? 0 : 1;
}
