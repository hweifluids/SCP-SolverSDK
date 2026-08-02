#pragma once

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace streamcenterplus::manifest {

struct VisualizationAxisValue {
    std::string key;
    std::string label;
    bool hasNumericValue = false;
    double numericValue = 0.0;
};

struct VisualizationAxis {
    std::string id;
    std::string label;
    std::string kind;
    std::string semantic;
    std::string unit;
    bool playable = false;
    std::string defaultValue;
    std::vector<VisualizationAxisValue> values;
};

struct VisualizationQuantity {
    std::string id;
    std::string label;
    std::string group;
    std::string arrayName;
    std::string association = "point";
    std::string valueKind = "continuous";
    std::vector<std::string> componentLabels;
    bool advanced = false;
};

struct VisualizationVariantQuantity {
    std::string quantityId;
    std::string arrayName;
    std::string association = "point";
};

struct VisualizationVariant {
    std::map<std::string, std::string> selectors;
    std::string path;
    int stepIndex = -1;
    bool hasReaderTime = false;
    double readerTime = 0.0;
    std::vector<VisualizationVariantQuantity> quantities;
    std::map<std::string, std::string> annotations;
};

struct VisualizationCatalog {
    std::string solverFamily;
    std::string solverVariant = "structured";
    std::string runId;
    std::vector<VisualizationAxis> axes;
    std::vector<VisualizationQuantity> quantities;
    std::vector<VisualizationVariant> variants;
};

inline std::string VisualizationJsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(ch) << std::dec;
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    return out.str();
}

inline std::string VisualizationQuote(const std::string& value) {
    return "\"" + VisualizationJsonEscape(value) + "\"";
}

inline std::string CanonicalSolverFamily(std::string solverName) {
    std::transform(solverName.begin(), solverName.end(), solverName.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const std::string rawSuffix = "_raw";
    if (solverName.size() >= rawSuffix.size()
        && solverName.compare(solverName.size() - rawSuffix.size(), rawSuffix.size(), rawSuffix) == 0) {
        solverName.resize(solverName.size() - rawSuffix.size());
    }
    if (solverName == "tdspod") {
        return "tdspod";
    }
    if (solverName == "fdspod") {
        return "fdspod";
    }
    if (solverName == "waveletpod") {
        return "waveletpod";
    }
    return solverName;
}

inline std::string SolverVariantFromName(std::string solverName) {
    std::transform(solverName.begin(), solverName.end(), solverName.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return solverName.size() >= 4 && solverName.compare(solverName.size() - 4, 4, "_raw") == 0
        ? "raw"
        : "structured";
}

inline std::string MakeVisualizationRunId() {
    static std::atomic<std::uint64_t> sequence{0};
    const auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    std::ostringstream out;
    out << ticks << "-" << sequence.fetch_add(1, std::memory_order_relaxed);
    return out.str();
}

inline VisualizationCatalog MakeVisualizationCatalog(const std::string& solverName) {
    VisualizationCatalog result;
    result.solverFamily = CanonicalSolverFamily(solverName);
    result.solverVariant = SolverVariantFromName(solverName);
    result.runId = MakeVisualizationRunId();
    return result;
}

inline void ValidateVisualizationCatalog(const VisualizationCatalog& catalog,
                                         const std::filesystem::path& outputDir) {
    if (catalog.solverFamily.empty()) {
        throw std::runtime_error("Visualization catalog solverFamily must not be empty.");
    }
    if (catalog.runId.empty()) {
        throw std::runtime_error("Visualization catalog runId must not be empty.");
    }

    std::map<std::string, const VisualizationAxis*> axes;
    for (const VisualizationAxis& axis : catalog.axes) {
        if (axis.id.empty() || !axes.emplace(axis.id, &axis).second) {
            throw std::runtime_error("Visualization catalog axis ids must be non-empty and unique.");
        }
        if (axis.kind != "fixed" && axis.kind != "numeric") {
            throw std::runtime_error("Visualization catalog axis kind must be fixed or numeric: " + axis.id);
        }
        std::set<std::string> values;
        for (const VisualizationAxisValue& value : axis.values) {
            if (value.key.empty() || !values.insert(value.key).second) {
                throw std::runtime_error("Visualization catalog axis values must be non-empty and unique: "
                                         + axis.id);
            }
            if (value.hasNumericValue && !std::isfinite(value.numericValue)) {
                throw std::runtime_error("Visualization catalog numeric axis values must be finite: " + axis.id);
            }
        }
        if (!axis.defaultValue.empty() && values.find(axis.defaultValue) == values.end()) {
            throw std::runtime_error("Visualization catalog axis default is not present in values: " + axis.id);
        }
    }

    std::set<std::string> quantityIds;
    for (const VisualizationQuantity& quantity : catalog.quantities) {
        if (quantity.id.empty() || !quantityIds.insert(quantity.id).second) {
            throw std::runtime_error("Visualization catalog quantity ids must be non-empty and unique.");
        }
        if (quantity.association != "point" && quantity.association != "cell"
            && quantity.association != "field") {
            throw std::runtime_error("Visualization catalog quantity association is invalid: " + quantity.id);
        }
    }

    std::set<std::string> coordinates;
    const std::filesystem::path absoluteOutput =
        std::filesystem::absolute(outputDir).lexically_normal();
    for (const VisualizationVariant& variant : catalog.variants) {
        if (variant.path.empty()) {
            throw std::runtime_error("Visualization catalog variant path must not be empty.");
        }
        const std::filesystem::path relativePath(variant.path);
        if (relativePath.is_absolute()) {
            throw std::runtime_error("Visualization catalog variant paths must be relative: " + variant.path);
        }
        const std::filesystem::path resolved = (absoluteOutput / relativePath).lexically_normal();
        const auto relativeResolved = resolved.lexically_relative(absoluteOutput);
        if (relativeResolved.empty() || *relativeResolved.begin() == "..") {
            throw std::runtime_error("Visualization catalog variant path escapes the output directory: "
                                     + variant.path);
        }
        if (!std::filesystem::is_regular_file(resolved)) {
            throw std::runtime_error("Visualization catalog variant target does not exist: " + resolved.string());
        }

        std::ostringstream coordinate;
        for (const auto& [axisId, value] : variant.selectors) {
            const auto axisIt = axes.find(axisId);
            if (axisIt == axes.end()) {
                throw std::runtime_error("Visualization catalog variant references an unknown axis: " + axisId);
            }
            const auto& axisValues = axisIt->second->values;
            const auto valueIt = std::find_if(axisValues.begin(), axisValues.end(), [&](const auto& candidate) {
                return candidate.key == value;
            });
            if (valueIt == axisValues.end()) {
                throw std::runtime_error("Visualization catalog variant references an unknown axis value: "
                                         + axisId + "=" + value);
            }
            coordinate << axisId << "=" << value << "\x1f";
        }
        if (!coordinates.insert(coordinate.str()).second) {
            throw std::runtime_error("Visualization catalog contains duplicate selector coordinates.");
        }
        for (const VisualizationVariantQuantity& mapping : variant.quantities) {
            if (quantityIds.find(mapping.quantityId) == quantityIds.end()) {
                throw std::runtime_error("Visualization catalog variant references an unknown quantity: "
                                         + mapping.quantityId);
            }
        }
    }
}

inline void WriteVisualizationStringMap(std::ostream& out,
                                        const std::map<std::string, std::string>& values,
                                        const std::string& indent) {
    out << "{";
    bool first = true;
    for (const auto& [key, value] : values) {
        out << (first ? "\n" : ",\n");
        first = false;
        out << indent << "  " << VisualizationQuote(key) << ": " << VisualizationQuote(value);
    }
    if (!first) {
        out << "\n" << indent;
    }
    out << "}";
}

inline void WriteVisualizationCatalog(std::ostream& out,
                                      const VisualizationCatalog& catalog,
                                      const std::string& indent) {
    out << indent << "{\n";
    out << indent << "  \"solver_family\": " << VisualizationQuote(catalog.solverFamily) << ",\n";
    out << indent << "  \"solver_variant\": " << VisualizationQuote(catalog.solverVariant) << ",\n";
    out << indent << "  \"run_id\": " << VisualizationQuote(catalog.runId) << ",\n";

    out << indent << "  \"axes\": [";
    for (std::size_t axisIndex = 0; axisIndex < catalog.axes.size(); ++axisIndex) {
        const VisualizationAxis& axis = catalog.axes[axisIndex];
        out << (axisIndex == 0 ? "\n" : ",\n");
        out << indent << "    {\n";
        out << indent << "      \"id\": " << VisualizationQuote(axis.id) << ",\n";
        out << indent << "      \"label\": " << VisualizationQuote(axis.label) << ",\n";
        out << indent << "      \"kind\": " << VisualizationQuote(axis.kind) << ",\n";
        out << indent << "      \"semantic\": " << VisualizationQuote(axis.semantic) << ",\n";
        out << indent << "      \"unit\": " << VisualizationQuote(axis.unit) << ",\n";
        out << indent << "      \"playable\": " << (axis.playable ? "true" : "false") << ",\n";
        out << indent << "      \"default\": " << VisualizationQuote(axis.defaultValue) << ",\n";
        out << indent << "      \"values\": [";
        for (std::size_t valueIndex = 0; valueIndex < axis.values.size(); ++valueIndex) {
            const VisualizationAxisValue& value = axis.values[valueIndex];
            out << (valueIndex == 0 ? "\n" : ",\n");
            out << indent << "        {\"key\": " << VisualizationQuote(value.key)
                << ", \"label\": " << VisualizationQuote(value.label);
            if (value.hasNumericValue) {
                out << ", \"numeric_value\": " << std::setprecision(17) << value.numericValue;
            }
            out << "}";
        }
        if (!axis.values.empty()) {
            out << "\n" << indent << "      ";
        }
        out << "]\n";
        out << indent << "    }";
    }
    if (!catalog.axes.empty()) {
        out << "\n" << indent << "  ";
    }
    out << "],\n";

    out << indent << "  \"quantities\": [";
    for (std::size_t index = 0; index < catalog.quantities.size(); ++index) {
        const VisualizationQuantity& quantity = catalog.quantities[index];
        out << (index == 0 ? "\n" : ",\n");
        out << indent << "    {\n";
        out << indent << "      \"id\": " << VisualizationQuote(quantity.id) << ",\n";
        out << indent << "      \"label\": " << VisualizationQuote(quantity.label) << ",\n";
        out << indent << "      \"group\": " << VisualizationQuote(quantity.group) << ",\n";
        out << indent << "      \"array\": " << VisualizationQuote(quantity.arrayName) << ",\n";
        out << indent << "      \"association\": " << VisualizationQuote(quantity.association) << ",\n";
        out << indent << "      \"value_kind\": " << VisualizationQuote(quantity.valueKind) << ",\n";
        out << indent << "      \"advanced\": " << (quantity.advanced ? "true" : "false") << ",\n";
        out << indent << "      \"component_labels\": [";
        for (std::size_t component = 0; component < quantity.componentLabels.size(); ++component) {
            out << (component == 0 ? "" : ", ") << VisualizationQuote(quantity.componentLabels[component]);
        }
        out << "]\n";
        out << indent << "    }";
    }
    if (!catalog.quantities.empty()) {
        out << "\n" << indent << "  ";
    }
    out << "],\n";

    out << indent << "  \"variants\": [";
    for (std::size_t index = 0; index < catalog.variants.size(); ++index) {
        const VisualizationVariant& variant = catalog.variants[index];
        out << (index == 0 ? "\n" : ",\n");
        out << indent << "    {\n";
        out << indent << "      \"selectors\": ";
        WriteVisualizationStringMap(out, variant.selectors, indent + "      ");
        out << ",\n";
        out << indent << "      \"path\": " << VisualizationQuote(variant.path) << ",\n";
        if (variant.stepIndex >= 0) {
            out << indent << "      \"step_index\": " << variant.stepIndex << ",\n";
        }
        if (variant.hasReaderTime) {
            out << indent << "      \"reader_time\": " << std::setprecision(17)
                << variant.readerTime << ",\n";
        }
        out << indent << "      \"quantities\": [";
        for (std::size_t mappingIndex = 0; mappingIndex < variant.quantities.size(); ++mappingIndex) {
            const VisualizationVariantQuantity& mapping = variant.quantities[mappingIndex];
            out << (mappingIndex == 0 ? "\n" : ",\n");
            out << indent << "        {\"quantity_id\": " << VisualizationQuote(mapping.quantityId)
                << ", \"array\": " << VisualizationQuote(mapping.arrayName)
                << ", \"association\": " << VisualizationQuote(mapping.association) << "}";
        }
        if (!variant.quantities.empty()) {
            out << "\n" << indent << "      ";
        }
        out << "],\n";
        out << indent << "      \"annotations\": ";
        WriteVisualizationStringMap(out, variant.annotations, indent + "      ");
        out << "\n";
        out << indent << "    }";
    }
    if (!catalog.variants.empty()) {
        out << "\n" << indent << "  ";
    }
    out << "]\n";
    out << indent << "}";
}

}  // namespace streamcenterplus::manifest
