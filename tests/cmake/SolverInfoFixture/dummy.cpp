#include <streamcenterplus/SolverInfo.h>

int main(int argc, char** argv) {
    return streamcenterplus::HandleSolverInfoRequest(argc, argv) ? 0 : 1;
}
