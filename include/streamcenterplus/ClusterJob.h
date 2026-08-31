#pragma once

#include <string>

namespace streamcenterplus {

inline constexpr const char* kClusterJobPackageSchema =
    "streamcenterplus.job_package.v1";
inline constexpr const char* kClusterJobControlFormat =
    "streamcenterplus_deck_key_value";
inline constexpr const char* kClusterJobArchiveFormat = "scpjob.tar";

struct ClusterJobHints {
    bool packageRequested = false;
    std::string packagePath;
    std::string payloadMode = "single_archive";
    std::string controlFormat = kClusterJobControlFormat;
    std::string partitionMode = "cluster_preflight";
    std::string scheduleMethod = "serial";
    std::string partitionMethod = "auto";
    std::string partitionEngine = "auto";
    std::string partitionGranularity = "auto";
    std::string partitionObjective = "balance_compute";
    std::string placementMethod = "auto";
    std::string scheduler = "auto";
    std::string queue;
    std::string walltime;
    int nodes = 0;
    int ranks = 0;
    int gpus = 0;
    int gpusPerNode = 0;
    int cpuThreadsPerRank = 0;
    double memoryGb = 0.0;
};

inline bool HasClusterJobHints(const ClusterJobHints& hints) noexcept {
    return hints.packageRequested || !hints.packagePath.empty() ||
           hints.nodes > 0 || hints.ranks > 0 || hints.gpus > 0 ||
           hints.gpusPerNode > 0 || hints.cpuThreadsPerRank > 0 ||
           hints.memoryGb > 0.0 || hints.scheduler != "auto" ||
           !hints.queue.empty() || !hints.walltime.empty() ||
           hints.partitionMode != "cluster_preflight" ||
           hints.scheduleMethod != "serial" ||
           hints.partitionMethod != "auto" ||
           hints.partitionEngine != "auto" ||
           hints.partitionGranularity != "auto" ||
           hints.partitionObjective != "balance_compute" ||
           hints.placementMethod != "auto";
}

}  // namespace streamcenterplus
