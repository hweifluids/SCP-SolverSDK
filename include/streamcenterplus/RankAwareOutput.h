#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
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
    std::string runId;
};

inline std::string FourDigitRank(int rank) {
    std::ostringstream stream;
    stream << "rank_" << std::setw(4) << std::setfill('0') << rank;
    return stream.str();
}

inline RankOutputContext MakeContext(const std::filesystem::path& outputDir,
                                     int rank,
                                     int worldSize,
                                     bool enableRankAwareOutput,
                                     const std::string& runId = {}) {
    RankOutputContext context;
    context.rootOutputDir = outputDir;
    context.rank = rank;
    context.worldSize = worldSize;
    context.enabled = enableRankAwareOutput && worldSize > 1;
    context.isCoordinator = rank == 0;
    context.runId = runId;
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

inline void RequireSafeField(const std::string& value) {
    if (value.find_first_of("\r\n") != std::string::npos) {
        throw std::runtime_error("Invalid newline in rank manifest field.");
    }
}

inline std::filesystem::path CheckedPayload(const std::filesystem::path& root,
                                             const std::string& relative) {
    const std::filesystem::path path(relative);
    RequireSafeField(relative);
    if (path.empty() || path.is_absolute()) throw std::runtime_error("Invalid shard path.");
    auto current = root;
    for (const auto& component : path) {
        if (component == "..") throw std::runtime_error("Shard path escapes output directory.");
        current /= component;
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(current))) {
            throw std::runtime_error("Shard payload must not contain symbolic links.");
        }
    }
    if (!std::filesystem::is_regular_file(current) || std::filesystem::file_size(current) == 0) {
        throw std::runtime_error("Shard payload is missing or empty: " + current.string());
    }
    return current;
}

inline std::map<std::string, std::string> ReadShardFields(const std::filesystem::path& path) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path))) {
        throw std::runtime_error("Unsafe shard manifest.");
    }
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot read shard manifest: " + path.string());
    std::map<std::string, std::string> fields;
    std::string line;
    while (std::getline(input, line)) {
        const auto delimiter = line.find(" = ");
        if (delimiter == std::string::npos ||
            !fields.emplace(line.substr(0, delimiter), line.substr(delimiter + 3)).second) {
            throw std::runtime_error("Malformed or duplicate shard manifest field.");
        }
    }
    if (!input.eof()) throw std::runtime_error("Shard manifest read failed.");
    return fields;
}

inline void ValidateRankShardManifest(const RankOutputContext& context,
                                      int rank,
                                      const std::string& solverName,
                                      const std::string& partitionPlanPath) {
    const auto directory = context.rootOutputDir / "shards" / FourDigitRank(rank);
    const auto fields = ReadShardFields(directory / "rank_manifest.scpa");
    auto require = [&](const std::string& key, const std::string& value) {
        const auto found = fields.find(key);
        if (found == fields.end() || found->second != value) {
            throw std::runtime_error("Rank shard identity/status mismatch: " + key);
        }
    };
    if (context.runId.empty()) throw std::runtime_error("Rank output requires a run identity.");
    require("schema", "streamcenterplus.rank_shard_manifest.v2");
    require("run_id", context.runId);
    require("status", "complete");
    require("solver", solverName);
    require("rank", std::to_string(rank));
    require("world_size", std::to_string(context.worldSize));
    require("partition_plan", partitionPlanPath);
    const auto countField = fields.find("file_count");
    if (countField == fields.end() || countField->second.empty() ||
        countField->second.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("Invalid shard file count.");
    }
    const auto count = std::stoull(countField->second);
    if (count == 0 || count > fields.size()) throw std::runtime_error("Invalid shard inventory.");
    for (unsigned long long i = 0; i < count; ++i) {
        const auto entry = fields.find("file." + std::to_string(i));
        if (entry == fields.end()) throw std::runtime_error("Incomplete shard inventory.");
        CheckedPayload(directory, entry->second);
    }
}

inline void WriteRankShardManifest(const RankOutputContext& context,
                                   const std::string& solverName,
                                   const std::string& partitionPlanPath,
                                   const std::vector<std::string>& generatedFiles) {
    std::filesystem::create_directories(context.effectiveOutputDir);
    const std::filesystem::path path = context.effectiveOutputDir / "rank_manifest.scpa";
    if (context.runId.empty()) throw std::runtime_error("Rank output requires a run identity.");
    RequireSafeField(context.runId);
    RequireSafeField(solverName);
    RequireSafeField(partitionPlanPath);
    for (const auto& file : generatedFiles) CheckedPayload(context.effectiveOutputDir, file);
    const auto temporary = context.effectiveOutputDir / "rank_manifest.scpa.partial";
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot write rank shard manifest: " + path.string());
    }
    out << "schema = streamcenterplus.rank_shard_manifest.v2\n";
    out << "run_id = " << context.runId << "\n";
    out << "status = complete\n";
    out << "created_at_utc = " << UtcTimestamp() << "\n";
    out << "solver = " << solverName << "\n";
    out << "rank = " << context.rank << "\n";
    out << "world_size = " << context.worldSize << "\n";
    out << "partition_plan = " << partitionPlanPath << "\n";
    out << "file_count = " << generatedFiles.size() << "\n";
    for (std::size_t i = 0; i < generatedFiles.size(); ++i) {
        out << "file." << i << " = " << generatedFiles[i] << "\n";
    }
    out.close();
    if (!out) throw std::runtime_error("Cannot finish rank shard manifest.");
    std::filesystem::rename(temporary, path);
}

inline void WriteAssemblyManifest(const RankOutputContext& context,
                                  const std::string& solverName,
                                  const std::string& partitionPlanPath) {
    if (!context.enabled || !context.isCoordinator) {
        return;
    }
    for (int rank = 0; rank < context.worldSize; ++rank) {
        ValidateRankShardManifest(context, rank, solverName, partitionPlanPath);
    }
    const std::filesystem::path assemblyDir = context.rootOutputDir / "assembly";
    std::filesystem::create_directories(assemblyDir);
    const std::filesystem::path path = assemblyDir / "rank_assembly.scpa";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot write rank assembly manifest: " + path.string());
    }
    out << "schema = streamcenterplus.rank_assembly.v2\n";
    out << "run_id = " << context.runId << "\n";
    out << "created_at_utc = " << UtcTimestamp() << "\n";
    out << "solver = " << solverName << "\n";
    out << "world_size = " << context.worldSize << "\n";
    out << "partition_plan = " << partitionPlanPath << "\n";
    out << "shard_root = shards\n";
    out << "assembly_policy = rank0_transaction\n";
    for (int rank = 0; rank < context.worldSize; ++rank) {
        out << "rank." << rank << ".manifest = "
            << (std::filesystem::path("..") / "shards" / FourDigitRank(rank) / "rank_manifest.scpa").generic_string()
            << "\n";
    }
}

}  // namespace streamcenterplus::rank_output

