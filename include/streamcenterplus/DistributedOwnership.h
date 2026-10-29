#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "PartitionPlan.h"

namespace streamcenterplus::partition {

enum class OwnershipLocation {
    Files,
    Points,
    Cells,
};

struct Ownership {
    int rank = 0;
    int worldSize = 1;
    int ghostLayers = 0;
    OwnershipLocation location = OwnershipLocation::Files;
    std::vector<Range> ownedRanges;
    std::vector<Assignment> assignments;

    std::size_t ownedItemCount() const {
        std::size_t total = 0;
        for (const Range& range : ownedRanges) {
            total += range.end > range.start ? range.end - range.start : 0;
        }
        return total;
    }
};

inline OwnershipLocation LocationForAssignmentKind(const std::string& kind) {
    if (kind == "point" || kind == "point_block" || kind == "point_adjacency") {
        return OwnershipLocation::Points;
    }
    if (kind == "cell" || kind == "cell_block" || kind == "cell_adjacency") {
        return OwnershipLocation::Cells;
    }
    return OwnershipLocation::Files;
}

inline bool AssignmentKindMatchesLocation(const std::string& kind, OwnershipLocation location) {
    return LocationForAssignmentKind(kind) == location;
}

inline Ownership BuildOwnership(const Plan& plan, int rank, OwnershipLocation preferredLocation) {
    Ownership ownership;
    ownership.rank = rank;
    ownership.worldSize = plan.rankCount;
    ownership.ghostLayers = plan.ghostLayers;
    ownership.location = preferredLocation;

    for (const Assignment& assignment : plan.assignments) {
        if (assignment.rank != rank || !assignment.hasRange()) {
            continue;
        }
        if (!AssignmentKindMatchesLocation(assignment.kind, preferredLocation)) {
            continue;
        }
        ownership.assignments.push_back(assignment);
        ownership.ownedRanges.push_back(assignment.range);
    }
    std::sort(ownership.ownedRanges.begin(), ownership.ownedRanges.end(), [](const Range& lhs, const Range& rhs) {
        return lhs.start < rhs.start || (lhs.start == rhs.start && lhs.end < rhs.end);
    });
    return ownership;
}

inline bool Contains(const Ownership& ownership, std::size_t item) {
    if (ownership.ownedRanges.empty()) {
        return true;
    }
    for (const Range& range : ownership.ownedRanges) {
        if (item >= range.start && item < range.end) {
            return true;
        }
    }
    return false;
}

}  // namespace streamcenterplus::partition

