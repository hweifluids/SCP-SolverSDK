#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace streamcenterplus::rank_output {

struct RankOutputContext {
    std::filesystem::path rootOutputDir;
    std::filesystem::path effectiveOutputDir;
    std::filesystem::path shardRelativeDir;
    int rank = 0;
    int worldSize = 1;
    bool enabled = false;
    bool isCoordinator = true;
};

inline std::string FourDigitRank(int rank) {
    std::ostringstream stream;
    stream << "rank_" << std::setw(4) << std::setfill('0') << rank;
    return stream.str();
}

inline RankOutputContext MakeContext(const std::filesystem::path& outputDir,
                                     int rank,
                                     int worldSize,
                                     bool enableRankAwareOutput) {
    RankOutputContext context;
    context.rootOutputDir = outputDir;
    context.rank = rank;
    context.worldSize = worldSize;
    context.enabled = enableRankAwareOutput && worldSize > 1;
    context.isCoordinator = rank == 0;
    if (context.enabled) {
        context.shardRelativeDir = std::filesystem::path("shards") / FourDigitRank(rank);
        context.effectiveOutputDir = outputDir / context.shardRelativeDir;
    } else {
        context.effectiveOutputDir = outputDir;
    }
    return context;
}

inline std::string UtcTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc {};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

inline void WriteRankShardManifest(const RankOutputContext& context,
                                   const std::string& solverName,
                                   const std::string& partitionPlanPath,
                                   const std::vector<std::string>& generatedFiles) {
    std::filesystem::create_directories(context.effectiveOutputDir);
    const std::filesystem::path path = context.effectiveOutputDir / "rank_manifest.scpa";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot write rank shard manifest: " + path.string());
    }
    out << "schema = streamcenterplus.rank_shard_manifest.v1\n";
    out << "created_at_utc = " << UtcTimestamp() << "\n";
    out << "solver = " << solverName << "\n";
    out << "rank = " << context.rank << "\n";
    out << "world_size = " << context.worldSize << "\n";
    out << "partition_plan = " << partitionPlanPath << "\n";
    out << "file_count = " << generatedFiles.size() << "\n";
    for (std::size_t i = 0; i < generatedFiles.size(); ++i) {
        out << "file." << i << " = " << generatedFiles[i] << "\n";
    }
}

inline void WriteAssemblyManifest(const RankOutputContext& context,
                                  const std::string& solverName,
                                  const std::string& partitionPlanPath) {
    if (!context.enabled || !context.isCoordinator) {
        return;
    }
    const std::filesystem::path assemblyDir = context.rootOutputDir / "assembly";
    std::filesystem::create_directories(assemblyDir);
    const std::filesystem::path path = assemblyDir / "rank_assembly.scpa";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot write rank assembly manifest: " + path.string());
    }
    out << "schema = streamcenterplus.rank_assembly.v1\n";
    out << "created_at_utc = " << UtcTimestamp() << "\n";
    out << "solver = " << solverName << "\n";
    out << "world_size = " << context.worldSize << "\n";
    out << "partition_plan = " << partitionPlanPath << "\n";
    out << "shard_root = shards\n";
    out << "assembly_policy = rank0_manifest\n";
    out << "vtk_wrapper_status = reserved\n";
    for (int rank = 0; rank < context.worldSize; ++rank) {
        out << "rank." << rank << ".manifest = "
            << (std::filesystem::path("..") / "shards" / FourDigitRank(rank) / "rank_manifest.scpa").generic_string()
            << "\n";
    }
}

}  // namespace streamcenterplus::rank_output

