#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "HardwareAwareness.h"

namespace streamcenterplus::partition {

struct Range {
    std::size_t start = 0;
    std::size_t end = 0;
};

struct Assignment {
    std::string path;
    std::string kind;
    Range range;
    long long bytes = 0;
    int rank = 0;
    std::string gpuDevice;

    bool hasRange() const {
        return range.end > range.start;
    }
};

struct Plan {
    std::filesystem::path path;
    std::string schema;
    std::string graphSource;
    std::string granularity;
    std::string ownership;
    std::string methodEffective;
    std::string engineEffective;
    int rankCount = 1;
    int ghostLayers = 0;
    std::vector<Assignment> assignments;
};

inline std::string Trim(std::string value) {
    const std::string whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

inline std::map<std::string, std::string> ReadKeyValueFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open partition plan: " + path.string());
    }
    std::map<std::string, std::string> fields;
    std::string line;
    while (std::getline(input, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) {
            line.resize(comment);
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = Trim(line.substr(0, eq));
        std::string value = Trim(line.substr(eq + 1));
        if (!key.empty()) {
            fields[key] = value;
        }
    }
    return fields;
}

inline int IntField(const std::map<std::string, std::string>& fields,
                    const std::string& key,
                    int fallback) {
    const auto it = fields.find(key);
    if (it == fields.end()) {
        return fallback;
    }
    try {
        return std::stoi(it->second);
    } catch (...) {
        return fallback;
    }
}

inline long long LongLongField(const std::map<std::string, std::string>& fields,
                               const std::string& key,
                               long long fallback) {
    const auto it = fields.find(key);
    if (it == fields.end()) {
        return fallback;
    }
    try {
        return std::stoll(it->second);
    } catch (...) {
        return fallback;
    }
}

inline std::string StringField(const std::map<std::string, std::string>& fields,
                               const std::string& key,
                               std::string fallback = {}) {
    const auto it = fields.find(key);
    return it == fields.end() ? std::move(fallback) : it->second;
}

inline std::filesystem::path ResolvePlanPath(const std::filesystem::path& configuredPath) {
    if (!configuredPath.empty()) {
        return configuredPath;
    }
    const char* env = std::getenv("SCP_PARTITION_PLAN");
    if (env != nullptr && *env != '\0') {
        return std::filesystem::path(env);
    }
    return {};
}

inline Plan LoadPlan(const std::filesystem::path& path) {
    const std::map<std::string, std::string> fields = ReadKeyValueFile(path);
    Plan plan;
    plan.path = path;
    plan.schema = StringField(fields, "schema");
    plan.graphSource = StringField(fields, "graph.source");
    plan.granularity = StringField(fields, "partition.granularity");
    plan.ownership = StringField(fields, "partition.ownership");
    plan.methodEffective = StringField(fields, "partition.method.effective");
    plan.engineEffective = StringField(fields, "partition.engine.effective");
    plan.rankCount = std::max(1, IntField(fields, "rank_count", 1));
    plan.ghostLayers = std::max(0, IntField(fields, "partition.ghost_layers", 0));

    const int assignmentCount = IntField(fields, "assignment_count", 0);
    plan.assignments.reserve(static_cast<std::size_t>(std::max(0, assignmentCount)));
    for (int index = 0; index < assignmentCount; ++index) {
        const std::string prefix = "assignment." + std::to_string(index) + ".";
        Assignment assignment;
        assignment.path = StringField(fields, prefix + "path");
        assignment.kind = StringField(fields, prefix + "kind");
        assignment.rank = IntField(fields, prefix + "rank", 0);
        assignment.bytes = LongLongField(fields, prefix + "bytes", 0);
        assignment.gpuDevice = StringField(fields, prefix + "gpu_device");
        const long long start = LongLongField(fields, prefix + "range_start", -1);
        const long long end = LongLongField(fields, prefix + "range_end", -1);
        if (start >= 0 && end > start) {
            assignment.range.start = static_cast<std::size_t>(start);
            assignment.range.end = static_cast<std::size_t>(end);
        }
        plan.assignments.push_back(std::move(assignment));
    }
    return plan;
}

inline std::vector<Assignment> AssignmentsForRank(const Plan& plan, int rank) {
    std::vector<Assignment> result;
    for (const Assignment& assignment : plan.assignments) {
        if (assignment.rank == rank) {
            result.push_back(assignment);
        }
    }
    return result;
}

inline int ResolveRank(int configuredRank) {
    if (configuredRank >= 0) {
        return configuredRank;
    }
    return DetectHardwareAwareness().mpiRank;
}

}  // namespace streamcenterplus::partition
