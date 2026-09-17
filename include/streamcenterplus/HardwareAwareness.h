#pragma once

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace streamcenterplus {

struct HardwareAwareness {
    std::string scheduler = "local";
    std::string schedulerJobId;
    int mpiWorldSize = 1;
    int mpiRank = 0;
    int mpiLocalRank = 0;
    std::string cudaVisibleDevicesRaw;
    std::vector<std::string> cudaVisibleDevices;
};

inline const char* EnvironmentValue(const char* name) noexcept {
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
}

inline int EnvironmentInt(const char* name, int fallback) {
    const char* value = EnvironmentValue(name);
    if (*value == '\0') {
        return fallback;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    return end == value ? fallback : static_cast<int>(parsed);
}

inline std::vector<std::string> SplitVisibleDevices(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch) {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }), value.end());
    std::vector<std::string> devices;
    std::stringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ',')) {
        if (!token.empty() && token != "NoDevFiles" && token != "void") {
            devices.push_back(token);
        }
    }
    return devices;
}

inline HardwareAwareness DetectHardwareAwareness() {
    HardwareAwareness info;

    if (*EnvironmentValue("SLURM_JOB_ID") != '\0') {
        info.scheduler = "slurm";
        info.schedulerJobId = EnvironmentValue("SLURM_JOB_ID");
    } else if (*EnvironmentValue("PBS_JOBID") != '\0') {
        info.scheduler = "pbs";
        info.schedulerJobId = EnvironmentValue("PBS_JOBID");
    } else if (*EnvironmentValue("LSB_JOBID") != '\0') {
        info.scheduler = "lsf";
        info.schedulerJobId = EnvironmentValue("LSB_JOBID");
    }

    info.mpiWorldSize = EnvironmentInt("OMPI_COMM_WORLD_SIZE", info.mpiWorldSize);
    info.mpiWorldSize = EnvironmentInt("PMIX_SIZE", info.mpiWorldSize);
    info.mpiWorldSize = EnvironmentInt("PMI_SIZE", info.mpiWorldSize);
    info.mpiWorldSize = EnvironmentInt("MV2_COMM_WORLD_SIZE", info.mpiWorldSize);
    info.mpiWorldSize = EnvironmentInt("SLURM_NTASKS", info.mpiWorldSize);

    info.mpiRank = EnvironmentInt("OMPI_COMM_WORLD_RANK", info.mpiRank);
    info.mpiRank = EnvironmentInt("PMIX_RANK", info.mpiRank);
    info.mpiRank = EnvironmentInt("PMI_RANK", info.mpiRank);
    info.mpiRank = EnvironmentInt("MV2_COMM_WORLD_RANK", info.mpiRank);
    info.mpiRank = EnvironmentInt("SLURM_PROCID", info.mpiRank);

    info.mpiLocalRank = EnvironmentInt("OMPI_COMM_WORLD_LOCAL_RANK", info.mpiLocalRank);
    info.mpiLocalRank = EnvironmentInt("PMIX_LOCAL_RANK", info.mpiLocalRank);
    info.mpiLocalRank = EnvironmentInt("PMI_LOCAL_RANK", info.mpiLocalRank);
    info.mpiLocalRank = EnvironmentInt("MV2_COMM_WORLD_LOCAL_RANK", info.mpiLocalRank);
    info.mpiLocalRank = EnvironmentInt("SLURM_LOCALID", info.mpiLocalRank);

    info.cudaVisibleDevicesRaw = EnvironmentValue("CUDA_VISIBLE_DEVICES");
    info.cudaVisibleDevices = SplitVisibleDevices(info.cudaVisibleDevicesRaw);
    return info;
}

inline int PreferredCudaDeviceOrdinal(const HardwareAwareness& info) {
    const int visibleCount = static_cast<int>(info.cudaVisibleDevices.size());
    if (visibleCount <= 0) {
        return 0;
    }
    const int localRank = std::max(0, info.mpiLocalRank);
    return localRank % visibleCount;
}

inline std::string HardwareAwarenessSummary(const HardwareAwareness& info) {
    std::ostringstream out;
    out << "scheduler=" << info.scheduler
        << ", job_id=" << info.schedulerJobId
        << ", mpi_world_size=" << info.mpiWorldSize
        << ", mpi_rank=" << info.mpiRank
        << ", mpi_local_rank=" << info.mpiLocalRank
        << ", cuda_visible_devices=" << info.cudaVisibleDevicesRaw
        << ", preferred_cuda_ordinal=" << PreferredCudaDeviceOrdinal(info);
    return out.str();
}

}  // namespace streamcenterplus
