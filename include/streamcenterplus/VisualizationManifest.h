#pragma once

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <locale>
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

inline void ValidateVisualizationUtf8(const std::string& value) {
    for (std::size_t index = 0; index < value.size();) {
        const unsigned char lead = static_cast<unsigned char>(value[index]);
        if (lead < 0x80) {
            ++index;
            continue;
        }

        std::size_t continuationCount = 0;
        std::uint32_t codePoint = 0;
        std::uint32_t minimumCodePoint = 0;
        if ((lead & 0xe0) == 0xc0) {
            continuationCount = 1;
            codePoint = lead & 0x1f;
            minimumCodePoint = 0x80;
        } else if ((lead & 0xf0) == 0xe0) {
            continuationCount = 2;
            codePoint = lead & 0x0f;
            minimumCodePoint = 0x800;
        } else if ((lead & 0xf8) == 0xf0) {
            continuationCount = 3;
            codePoint = lead & 0x07;
            minimumCodePoint = 0x10000;
        } else {
            throw std::runtime_error("Visualization manifest strings must contain valid UTF-8.");
        }
        if (index + continuationCount >= value.size()) {
            throw std::runtime_error("Visualization manifest strings must contain valid UTF-8.");
        }
        for (std::size_t offset = 1; offset <= continuationCount; ++offset) {
            const unsigned char continuation =
                static_cast<unsigned char>(value[index + offset]);
            if ((continuation & 0xc0) != 0x80) {
                throw std::runtime_error("Visualization manifest strings must contain valid UTF-8.");
            }
            codePoint = (codePoint << 6) | (continuation & 0x3f);
        }
        if (codePoint < minimumCodePoint || codePoint > 0x10ffff
            || (codePoint >= 0xd800 && codePoint <= 0xdfff)) {
            throw std::runtime_error("Visualization manifest strings must contain valid UTF-8.");
        }
        index += continuationCount + 1;
    }
}

inline std::uint32_t VisualizationUtf8CodePointAt(const std::string& value,
                                                   std::size_t index,
                                                   std::size_t* nextIndex) {
    const unsigned char lead = static_cast<unsigned char>(value[index]);
    std::size_t continuationCount = 0;
    std::uint32_t codePoint = lead;
    if ((lead & 0xe0) == 0xc0) {
        continuationCount = 1;
        codePoint = lead & 0x1f;
    } else if ((lead & 0xf0) == 0xe0) {
        continuationCount = 2;
        codePoint = lead & 0x0f;
    } else if ((lead & 0xf8) == 0xf0) {
        continuationCount = 3;
        codePoint = lead & 0x07;
    }
    for (std::size_t offset = 1; offset <= continuationCount; ++offset) {
        codePoint = (codePoint << 6)
            | (static_cast<unsigned char>(value[index + offset]) & 0x3f);
    }
    *nextIndex = index + continuationCount + 1;
    return codePoint;
}

inline bool VisualizationIsUnicodeWhitespace(std::uint32_t codePoint) {
    return (codePoint >= 0x09 && codePoint <= 0x0d) || codePoint == 0x20
        || codePoint == 0x85 || codePoint == 0xa0 || codePoint == 0x1680
        || (codePoint >= 0x2000 && codePoint <= 0x200a)
        || codePoint == 0x2028 || codePoint == 0x2029 || codePoint == 0x202f
        || codePoint == 0x205f || codePoint == 0x3000;
}

inline bool VisualizationHasBoundaryWhitespace(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    ValidateVisualizationUtf8(value);
    std::size_t index = 0;
    std::uint32_t first = 0;
    std::uint32_t last = 0;
    bool hasFirst = false;
    while (index < value.size()) {
        const std::uint32_t codePoint = VisualizationUtf8CodePointAt(value, index, &index);
        if (!hasFirst) {
            first = codePoint;
            hasFirst = true;
        }
        last = codePoint;
    }
    return VisualizationIsUnicodeWhitespace(first) || VisualizationIsUnicodeWhitespace(last);
}

inline void ValidateVisualizationIdentifier(const std::string& value,
                                            const std::string& description) {
    ValidateVisualizationUtf8(value);
    if (value.empty() || VisualizationHasBoundaryWhitespace(value)) {
        throw std::runtime_error(description + " must be non-empty without boundary whitespace.");
    }
}

inline std::string VisualizationJsonEscape(const std::string& value) {
    ValidateVisualizationUtf8(value);
    std::ostringstream out;
    out.imbue(std::locale::classic());
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

inline std::string VisualizationJsonNumber(double value) {
    if (!std::isfinite(value)) {
        throw std::runtime_error("Visualization manifest JSON numbers must be finite.");
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::defaultfloat << std::setprecision(std::numeric_limits<double>::max_digits10)
        << value;
    return out.str();
}

inline bool VisualizationKeyIsFiniteNumber(const std::string& value) {
    std::size_t index = 0;
    if (index < value.size() && (value[index] == '+' || value[index] == '-')) {
        ++index;
    }
    bool hasMantissaDigit = false;
    while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
        hasMantissaDigit = true;
        ++index;
    }
    if (index < value.size() && value[index] == '.') {
        ++index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
            hasMantissaDigit = true;
            ++index;
        }
    }
    if (!hasMantissaDigit) {
        return false;
    }
    if (index < value.size() && (value[index] == 'e' || value[index] == 'E')) {
        ++index;
        if (index < value.size() && (value[index] == '+' || value[index] == '-')) {
            ++index;
        }
        const std::size_t exponentStart = index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
            ++index;
        }
        if (index == exponentStart) {
            return false;
        }
    }
    if (index != value.size()) {
        return false;
    }
    std::istringstream input(value);
    input.imbue(std::locale::classic());
    input >> std::noskipws;
    double parsed = 0.0;
    input >> parsed;
    return input && input.peek() == std::char_traits<char>::eof() && std::isfinite(parsed);
}

inline std::string VisualizationPathToUtf8(const std::filesystem::path& path) {
    const auto encoded = path.generic_u8string();
    std::string result;
    result.reserve(encoded.size());
    for (const auto ch : encoded) {
        result.push_back(static_cast<char>(ch));
    }
    return result;
}

inline std::filesystem::path VisualizationPathFromUtf8(const std::string& value) {
#if defined(__cpp_char8_t)
    std::u8string encoded;
    encoded.reserve(value.size());
    for (const unsigned char ch : value) {
        encoded.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path(encoded);
#else
    return std::filesystem::u8path(value);
#endif
}

inline bool VisualizationPathIsWithin(const std::filesystem::path& path,
                                      const std::filesystem::path& directory) {
    std::filesystem::path candidate = path;
    while (!candidate.empty()) {
        std::error_code error;
        if (std::filesystem::equivalent(candidate, directory, error) && !error) {
            return true;
        }
        const std::filesystem::path parent = candidate.parent_path();
        if (parent == candidate) {
            break;
        }
        candidate = parent;
    }
    return false;
}

inline void ValidateVisualizationFileWithinOutput(
    const std::filesystem::path& path,
    const std::filesystem::path& outputDir,
    const std::string& displayPath) {
    std::error_code error;
    const std::filesystem::path canonicalOutput =
        std::filesystem::weakly_canonical(std::filesystem::absolute(outputDir), error);
    if (error) {
        throw std::runtime_error("Cannot resolve the visualization output directory: "
                                 + VisualizationPathToUtf8(outputDir) + ": " + error.message());
    }
    const std::filesystem::path canonicalTarget = std::filesystem::canonical(path, error);
    if (error || !std::filesystem::is_regular_file(canonicalTarget)) {
        throw std::runtime_error("Visualization catalog variant target does not exist: "
                                 + displayPath);
    }
    if (!VisualizationPathIsWithin(canonicalTarget, canonicalOutput)) {
        throw std::runtime_error("Visualization catalog variant path escapes the output directory: "
                                 + displayPath);
    }
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
    out.imbue(std::locale::classic());
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
    ValidateVisualizationIdentifier(catalog.solverFamily,
                                    "Visualization catalog solverFamily");
    ValidateVisualizationUtf8(catalog.solverVariant);
    if (!catalog.solverVariant.empty()
        && VisualizationHasBoundaryWhitespace(catalog.solverVariant)) {
        throw std::runtime_error(
            "Visualization catalog solverVariant must not have boundary whitespace.");
    }
    ValidateVisualizationIdentifier(catalog.runId, "Visualization catalog runId");

    std::map<std::string, const VisualizationAxis*> axes;
    for (const VisualizationAxis& axis : catalog.axes) {
        ValidateVisualizationIdentifier(axis.id, "Visualization catalog axis id");
        ValidateVisualizationUtf8(axis.label);
        ValidateVisualizationUtf8(axis.kind);
        ValidateVisualizationUtf8(axis.semantic);
        ValidateVisualizationUtf8(axis.unit);
        ValidateVisualizationUtf8(axis.defaultValue);
        if (!axes.emplace(axis.id, &axis).second) {
            throw std::runtime_error("Visualization catalog axis ids must be unique.");
        }
        if (axis.kind != "fixed" && axis.kind != "numeric") {
            throw std::runtime_error("Visualization catalog axis kind must be fixed or numeric: " + axis.id);
        }
        std::set<std::string> values;
        for (const VisualizationAxisValue& value : axis.values) {
            ValidateVisualizationUtf8(value.key);
            ValidateVisualizationUtf8(value.label);
            if (value.key.empty() || !values.insert(value.key).second) {
                throw std::runtime_error("Visualization catalog axis values must be non-empty and unique: "
                                         + axis.id);
            }
            if (value.hasNumericValue && !std::isfinite(value.numericValue)) {
                throw std::runtime_error("Visualization catalog numeric axis values must be finite: " + axis.id);
            }
            if (axis.kind == "numeric" && !value.hasNumericValue
                && !VisualizationKeyIsFiniteNumber(value.key)) {
                throw std::runtime_error(
                    "Visualization catalog numeric axis values require numeric_value when the key is not numeric: "
                    + axis.id + "=" + value.key);
            }
        }
        if (!axis.defaultValue.empty() && values.find(axis.defaultValue) == values.end()) {
            throw std::runtime_error("Visualization catalog axis default is not present in values: " + axis.id);
        }
    }

    std::set<std::string> quantityIds;
    for (const VisualizationQuantity& quantity : catalog.quantities) {
        ValidateVisualizationIdentifier(quantity.id, "Visualization catalog quantity id");
        ValidateVisualizationUtf8(quantity.label);
        ValidateVisualizationUtf8(quantity.group);
        ValidateVisualizationUtf8(quantity.arrayName);
        ValidateVisualizationUtf8(quantity.association);
        ValidateVisualizationUtf8(quantity.valueKind);
        for (const std::string& componentLabel : quantity.componentLabels) {
            ValidateVisualizationUtf8(componentLabel);
        }
        if (!quantityIds.insert(quantity.id).second) {
            throw std::runtime_error("Visualization catalog quantity ids must be unique.");
        }
        if (quantity.arrayName.find('\0') != std::string::npos
            || (!quantity.arrayName.empty()
                && VisualizationHasBoundaryWhitespace(quantity.arrayName))) {
            throw std::runtime_error(
                "Visualization catalog quantity arrayName must not contain embedded NUL or boundary whitespace: "
                + quantity.id);
        }
        if (quantity.association != "point" && quantity.association != "cell"
            && quantity.association != "field") {
            throw std::runtime_error("Visualization catalog quantity association is invalid: " + quantity.id);
        }
    }

    std::set<std::map<std::string, std::string>> coordinates;
    const std::filesystem::path absoluteOutput =
        std::filesystem::absolute(outputDir).lexically_normal();
    for (const VisualizationVariant& variant : catalog.variants) {
        ValidateVisualizationUtf8(variant.path);
        if (variant.path.empty() || variant.path.find('\0') != std::string::npos
            || VisualizationHasBoundaryWhitespace(variant.path)) {
            throw std::runtime_error(
                "Visualization catalog variant path must be non-empty without embedded NUL or boundary whitespace.");
        }
        const std::filesystem::path relativePath = VisualizationPathFromUtf8(variant.path);
        if (relativePath.is_absolute()) {
            throw std::runtime_error("Visualization catalog variant paths must be relative: " + variant.path);
        }
        const std::filesystem::path resolved = (absoluteOutput / relativePath).lexically_normal();
        const auto relativeResolved = resolved.lexically_relative(absoluteOutput);
        if (relativeResolved.empty() || *relativeResolved.begin() == "..") {
            throw std::runtime_error("Visualization catalog variant path escapes the output directory: "
                                     + variant.path);
        }
        ValidateVisualizationFileWithinOutput(resolved, outputDir, variant.path);
        if (variant.hasReaderTime && !std::isfinite(variant.readerTime)) {
            throw std::runtime_error("Visualization catalog variant readerTime must be finite.");
        }

        for (const auto& [axisId, value] : variant.selectors) {
            ValidateVisualizationUtf8(axisId);
            ValidateVisualizationUtf8(value);
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
        }
        if (!coordinates.insert(variant.selectors).second) {
            throw std::runtime_error("Visualization catalog contains duplicate selector coordinates.");
        }
        std::set<std::string> mappedQuantityIds;
        for (const VisualizationVariantQuantity& mapping : variant.quantities) {
            ValidateVisualizationUtf8(mapping.quantityId);
            ValidateVisualizationUtf8(mapping.arrayName);
            ValidateVisualizationUtf8(mapping.association);
            if (quantityIds.find(mapping.quantityId) == quantityIds.end()) {
                throw std::runtime_error("Visualization catalog variant references an unknown quantity: "
                                         + mapping.quantityId);
            }
            if (mapping.arrayName.empty()
                || mapping.arrayName.find('\0') != std::string::npos
                || VisualizationHasBoundaryWhitespace(mapping.arrayName)) {
                throw std::runtime_error(
                    "Visualization catalog variant quantity arrayName must be non-empty without embedded NUL or boundary whitespace: "
                    + mapping.quantityId);
            }
            if (!mappedQuantityIds.insert(mapping.quantityId).second) {
                throw std::runtime_error(
                    "Visualization catalog variant maps a quantity more than once: "
                    + mapping.quantityId);
            }
        }
        for (const auto& [key, value] : variant.annotations) {
            ValidateVisualizationUtf8(key);
            ValidateVisualizationUtf8(value);
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
                out << ", \"numeric_value\": " << VisualizationJsonNumber(value.numericValue);
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
            out << indent << "      \"step_index\": " << std::to_string(variant.stepIndex)
                << ",\n";
        }
        if (variant.hasReaderTime) {
            out << indent << "      \"reader_time\": "
                << VisualizationJsonNumber(variant.readerTime) << ",\n";
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
