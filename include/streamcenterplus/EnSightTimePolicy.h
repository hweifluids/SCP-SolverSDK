#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <limits>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace streamcenterplus::ensight_time {

inline constexpr const char* kTimeQuantumAttribute =
    "streamcenterplus_ensight_time_quantum";
inline constexpr double kMaximumTimeQuantumFraction = 0.011;

struct TimeAxisAssessment {
    bool valid = false;
    bool approximate = false;
    double interval = 1.0;
    double maximumResidual = 0.0;
};

inline double TimeRoundoff(double left, double right) {
    return 8.0 * std::numeric_limits<double>::epsilon() *
               std::max(std::abs(left), std::abs(right)) +
           16.0 * std::numeric_limits<double>::denorm_min();
}

inline TimeAxisAssessment AssessTimeAxis(const std::vector<double>& times,
                                         double declaredQuantum = 0.0) {
    TimeAxisAssessment result;
    if (!std::all_of(times.begin(), times.end(),
                     [](double value) { return std::isfinite(value); })) {
        return result;
    }
    if (times.size() <= 1) {
        result.valid = true;
        return result;
    }
    const double firstStep = times[1] - times[0];
    if (!std::isfinite(firstStep) || firstStep <= 0.0) {
        return result;
    }
    bool strict = true;
    for (std::size_t index = 2; index < times.size(); ++index) {
        const double expected = std::fma(static_cast<double>(index), firstStep, times[0]);
        const double tolerance = std::abs(firstStep) * static_cast<double>(index) * 1.0e-10 +
                                 TimeRoundoff(times[index], expected);
        if (!(times[index] > times[index - 1]) || !std::isfinite(expected) ||
            std::abs(times[index] - expected) > tolerance) {
            strict = false;
            break;
        }
    }
    if (strict) {
        result.valid = true;
        result.interval = firstStep;
        return result;
    }
    if (times.size() < 3 || !std::isfinite(declaredQuantum) || declaredQuantum <= 0.0) {
        return result;
    }
    const double effectiveInterval = (times.back() - times.front()) /
                                     static_cast<double>(times.size() - 1);
    if (!std::isfinite(effectiveInterval) || effectiveInterval <= 0.0 ||
        declaredQuantum > kMaximumTimeQuantumFraction * effectiveInterval) {
        return result;
    }
    for (std::size_t index = 0; index < times.size(); ++index) {
        const double expected =
            std::fma(static_cast<double>(index), effectiveInterval, times.front());
        const double roundoff = TimeRoundoff(times[index], expected);
        const double residual = std::abs(times[index] - expected);
        if (!std::isfinite(expected) || residual > declaredQuantum + roundoff) {
            return result;
        }
        if (index > 0) {
            const double step = times[index] - times[index - 1];
            if (!std::isfinite(step) || step <= 0.0 ||
                std::abs(step - effectiveInterval) > 2.0 * declaredQuantum + roundoff) {
                return result;
            }
        }
        result.maximumResidual = std::max(result.maximumResidual, residual);
    }
    result.valid = true;
    result.approximate = true;
    result.interval = effectiveInterval;
    return result;
}

inline double DecimalTimeQuantum(const std::vector<std::string>& tokens) {
    static const std::regex decimal(
        R"(^[+-]?(?:[0-9]+(?:\.([0-9]*))?|\.([0-9]+))(?:[eE]([+-]?[0-9]+))?$)");
    double quantum = std::numeric_limits<double>::infinity();
    for (const std::string& token : tokens) {
        std::smatch match;
        if (!std::regex_match(token, match, decimal)) {
            return 0.0;
        }
        int exponent = 0;
        try {
            if (match[3].matched) {
                exponent = std::stoi(match[3].str());
            }
        } catch (const std::exception&) {
            return 0.0;
        }
        const std::size_t fractionalDigits = std::max(match[1].length(), match[2].length());
        const double tokenQuantum =
            std::pow(10.0, static_cast<double>(exponent) - fractionalDigits);
        if (!std::isfinite(tokenQuantum) || tokenQuantum <= 0.0) {
            return 0.0;
        }
        quantum = std::min(quantum, tokenQuantum);
    }
    return std::isfinite(quantum) ? quantum : 0.0;
}

// Read only the real XML root; a commented or nested tag cannot introduce
// EnSight provenance into an otherwise ordinary PVD source.
inline std::optional<double> PvdTimeQuantum(const std::string& xml) {
    std::size_t position = 0;
    for (;;) {
        position = xml.find('<', position);
        if (position == std::string::npos) {
            return std::nullopt;
        }
        const bool comment = xml.compare(position, 4, "<!--") == 0;
        const bool instruction = xml.compare(position, 2, "<?") == 0;
        if (comment || instruction) {
            const std::string ending = comment ? "-->" : "?>";
            const std::size_t end = xml.find(ending, position + 2);
            if (end == std::string::npos) {
                throw std::runtime_error("Malformed PVD XML before its root element.");
            }
            position = end + ending.size();
            continue;
        }
        ++position;
        const auto whitespace = [&]() {
            while (position < xml.size() &&
                   std::isspace(static_cast<unsigned char>(xml[position]))) {
                ++position;
            }
        };
        const auto nameCharacter = [](char value) {
            return std::isalnum(static_cast<unsigned char>(value)) ||
                   value == '_' || value == '-' || value == ':' || value == '.';
        };
        whitespace();
        const std::size_t rootStart = position;
        while (position < xml.size() && nameCharacter(xml[position])) {
            ++position;
        }
        std::string rootName = xml.substr(rootStart, position - rootStart);
        std::transform(rootName.begin(), rootName.end(), rootName.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (rootName != "vtkfile") {
            return std::nullopt;
        }
        std::optional<double> result;
        for (;;) {
            whitespace();
            if (position >= xml.size()) {
                throw std::runtime_error("Unterminated PVD root element.");
            }
            if (xml[position] == '>' || xml[position] == '/') {
                return result;
            }
            const std::size_t attributeStart = position;
            while (position < xml.size() && nameCharacter(xml[position])) {
                ++position;
            }
            const std::string attribute = xml.substr(attributeStart, position - attributeStart);
            whitespace();
            if (attribute.empty() || position >= xml.size() || xml[position++] != '=') {
                throw std::runtime_error("Malformed PVD root attribute.");
            }
            whitespace();
            if (position >= xml.size() || (xml[position] != '\'' && xml[position] != '"')) {
                throw std::runtime_error("Unquoted PVD root attribute.");
            }
            const char quote = xml[position++];
            const std::size_t valueStart = position;
            while (position < xml.size() && xml[position] != quote) {
                ++position;
            }
            if (position >= xml.size()) {
                throw std::runtime_error("Unterminated PVD root attribute.");
            }
            const std::string value = xml.substr(valueStart, position++ - valueStart);
            if (attribute != kTimeQuantumAttribute) {
                continue;
            }
            if (result.has_value()) {
                throw std::runtime_error("Duplicate EnSight time quantum in PVD root.");
            }
            try {
                std::size_t consumed = 0;
                const double quantum = std::stod(value, &consumed);
                if (consumed != value.size() || !std::isfinite(quantum) || quantum <= 0.0) {
                    throw std::runtime_error("invalid quantum");
                }
                result = quantum;
            } catch (const std::exception&) {
                throw std::runtime_error("Invalid EnSight time quantum in PVD root.");
            }
        }
    }
}

// Created only after validating a complete, approximate EnSight source axis.
// Keep that axis origin and interval when selecting a one/two-frame subset;
// its rounded endpoints must not silently redefine the solver's physical dt.
struct TimeFrameProvenance {
    double quantum = 0.0;
    double interval = 0.0;
    double origin = 0.0;
    std::size_t frameCount = 0;
    std::size_t frameIndex = 0;
};

inline std::optional<double> ValidatedTimeStep(
    const std::vector<double>& times,
    const std::vector<TimeFrameProvenance>& provenance) {
    if (times.empty() || times.size() != provenance.size()) {
        return std::nullopt;
    }
    const TimeFrameProvenance& axis = provenance.front();
    if (!std::isfinite(axis.quantum) || axis.quantum <= 0.0 ||
        !std::isfinite(axis.interval) || axis.interval <= 0.0 ||
        !std::isfinite(axis.origin) || axis.frameCount < 3 ||
        axis.quantum > kMaximumTimeQuantumFraction * axis.interval) {
        return std::nullopt;
    }
    const std::size_t stride = times.size() > 1 &&
                                      provenance[1].frameIndex > axis.frameIndex
                                  ? provenance[1].frameIndex - axis.frameIndex
                                  : 1;
    for (std::size_t index = 0; index < times.size(); ++index) {
        const TimeFrameProvenance& frame = provenance[index];
        const double expected = std::fma(static_cast<double>(frame.frameIndex),
                                         axis.interval, axis.origin);
        if (frame.quantum != axis.quantum || frame.interval != axis.interval ||
            frame.origin != axis.origin || frame.frameCount != axis.frameCount ||
            frame.frameIndex >= axis.frameCount || !std::isfinite(times[index]) ||
            !std::isfinite(expected) ||
            std::abs(times[index] - expected) >
                axis.quantum + TimeRoundoff(times[index], expected) ||
            (index > 0 && (frame.frameIndex <= provenance[index - 1].frameIndex ||
                           frame.frameIndex - provenance[index - 1].frameIndex != stride ||
                           !(times[index] > times[index - 1])))) {
            return std::nullopt;
        }
    }
    const double selectedInterval = axis.interval * static_cast<double>(stride);
    return std::isfinite(selectedInterval) ? std::optional<double>(selectedInterval)
                                          : std::nullopt;
}

}  // namespace streamcenterplus::ensight_time
