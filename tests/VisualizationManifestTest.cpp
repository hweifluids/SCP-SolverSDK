#include <streamcenterplus/VisualizationManifest.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

namespace {

namespace manifest = streamcenterplus::manifest;

struct TemporaryDirectory {
    explicit TemporaryDirectory(std::string suffix)
        : path(std::filesystem::temp_directory_path()
               / ("scp-visualization-manifest-" + manifest::MakeVisualizationRunId()
                  + "-" + std::move(suffix))) {
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

class NonJsonNumberPunctuation : public std::numpunct<char> {
protected:
    char do_decimal_point() const override {
        return ',';
    }

    char do_thousands_sep() const override {
        return '_';
    }

    std::string do_grouping() const override {
        return "\3";
    }
};

class ScopedGlobalLocale {
public:
    explicit ScopedGlobalLocale(const std::locale& replacement)
        : previous_(std::locale::global(replacement)) {}

    ~ScopedGlobalLocale() {
        std::locale::global(previous_);
    }

private:
    std::locale previous_;
};

int Fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

bool ThrowsContaining(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::exception& error) {
        return expected.empty() || std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

manifest::VisualizationCatalog MakeValidCatalog() {
    manifest::VisualizationCatalog catalog = manifest::MakeVisualizationCatalog("DFT");
    catalog.axes.push_back({
        "rank",
        "Rank",
        "numeric",
        "rank",
        "",
        true,
        "1",
        {{"1", "rank=1", true, 1.0}, {"2", "rank=2", true, 2.0}},
    });
    catalog.quantities.push_back({
        "mode_magnitude",
        "Mode magnitude",
        "Mode",
        "phi_velocity_magnitude",
        "point",
        "continuous",
        {},
        false,
    });
    manifest::VisualizationVariant variant;
    variant.selectors.emplace("rank", "1");
    variant.path = "mode.vti";
    variant.quantities.push_back({"mode_magnitude", "phi_velocity_magnitude", "point"});
    variant.annotations.emplace("frequency", "12.5");
    catalog.variants.push_back(std::move(variant));
    return catalog;
}

}  // namespace

int main() {
    if (manifest::CanonicalSolverFamily("fdSPOD_RAW") != "fdspod") {
        return Fail("fdSPOD_RAW did not normalize to fdspod.");
    }
    if (manifest::SolverVariantFromName("TOD_RAW") != "raw") {
        return Fail("TOD_RAW did not normalize to the raw variant.");
    }

    TemporaryDirectory output("output");
    {
        std::ofstream data(output.path / "mode.vti");
        data << "fixture";
    }

    const manifest::VisualizationCatalog validCatalog = MakeValidCatalog();
    try {
        manifest::ValidateVisualizationCatalog(validCatalog, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A valid catalog was rejected: ") + error.what());
    }

    std::ostringstream json;
    manifest::WriteVisualizationCatalog(json, validCatalog, "  ");
    const std::string payload = json.str();
    if (payload.find("\"solver_family\": \"dft\"") == std::string::npos
        || payload.find("\"numeric_value\": 1") == std::string::npos
        || payload.find("\"quantity_id\": \"mode_magnitude\"") == std::string::npos) {
        return Fail("Serialized catalog omitted required semantic fields.");
    }

    std::string controlThenHex;
    controlThenHex.push_back('\x01');
    controlThenHex += "ABC";
    if (manifest::VisualizationJsonEscape(controlThenHex) != "\\u0001ABC") {
        return Fail("A C0 control character was not emitted as a fixed-width JSON escape.");
    }
    if (!ThrowsContaining(
            [] { manifest::VisualizationJsonEscape(std::string(1, static_cast<char>(0xc3))); },
            "valid UTF-8")) {
        return Fail("An incomplete UTF-8 sequence was accepted by the JSON writer.");
    }
    for (const std::string& invalidUtf8 : {
             std::string("\xc0\x80", 2),
             std::string("\xed\xa0\x80", 3),
             std::string("\xf4\x90\x80\x80", 4),
         }) {
        if (!ThrowsContaining(
                [&] { manifest::VisualizationJsonEscape(invalidUtf8); },
                "valid UTF-8")) {
            return Fail("An overlong, surrogate, or out-of-range UTF-8 sequence was accepted.");
        }
    }

    manifest::VisualizationCatalog unknownValue = validCatalog;
    unknownValue.variants.front().selectors["rank"] = "3";
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(unknownValue, output.path); },
            "unknown axis value")) {
        return Fail("An unknown numeric axis value was accepted.");
    }

    manifest::VisualizationCatalog nulPath = validCatalog;
    nulPath.variants.front().path = std::string("mode.vti\0ignored", 16);
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(nulPath, output.path); },
            "embedded NUL")) {
        return Fail("A variant path containing an embedded NUL was accepted.");
    }

    manifest::VisualizationCatalog unicodeBoundaryWhitespace = validCatalog;
    unicodeBoundaryWhitespace.axes.front().id = "\xc2\xa0rank";
    unicodeBoundaryWhitespace.variants.front().selectors = {
        {"\xc2\xa0rank", "1"},
    };
    if (!ThrowsContaining(
            [&] {
                manifest::ValidateVisualizationCatalog(unicodeBoundaryWhitespace, output.path);
            },
            "boundary whitespace")) {
        return Fail("A catalog identifier with Unicode boundary whitespace was accepted.");
    }

    manifest::VisualizationCatalog conditionalAxis = validCatalog;
    conditionalAxis.axes.push_back({
        "phase", "Phase", "fixed", "phase", "", false, "a", {{"a", "A"}, {"b", "B"}},
    });
    try {
        manifest::ValidateVisualizationCatalog(conditionalAxis, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A variant that omitted an inapplicable conditional axis was rejected: ")
                    + error.what());
    }

    manifest::VisualizationCatalog sparse = conditionalAxis;
    manifest::VisualizationVariant second = sparse.variants.front();
    second.selectors["rank"] = "2";
    second.selectors["phase"] = "b";
    sparse.variants.push_back(std::move(second));
    try {
        manifest::ValidateVisualizationCatalog(sparse, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A sparse set of selector tuples was rejected: ")
                    + error.what());
    }

    manifest::VisualizationCatalog noAxes = manifest::MakeVisualizationCatalog("DFT");
    manifest::VisualizationVariant noAxisVariant;
    noAxisVariant.path = "mode.vti";
    noAxes.variants.push_back(std::move(noAxisVariant));
    try {
        manifest::ValidateVisualizationCatalog(noAxes, output.path);
        noAxes.variants.clear();
        manifest::ValidateVisualizationCatalog(noAxes, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A catalog without selector axes was rejected: ") + error.what());
    }

    const std::string unitSeparator(1, '\x1f');
    manifest::VisualizationCatalog collision = manifest::MakeVisualizationCatalog("DFT");
    collision.axes.push_back({
        "a",
        "A",
        "fixed",
        "",
        "",
        false,
        "x",
        {{"x", "x"}, {"x" + unitSeparator + "b=p", "encoded"}},
    });
    collision.axes.push_back({
        "b",
        "B",
        "fixed",
        "",
        "",
        false,
        "q",
        {{"p" + unitSeparator + "b=q", "encoded"}, {"q", "q"}},
    });
    manifest::VisualizationVariant collisionFirst;
    collisionFirst.path = "mode.vti";
    collisionFirst.selectors = {{"a", "x"}, {"b", "p" + unitSeparator + "b=q"}};
    collision.variants.push_back(collisionFirst);
    manifest::VisualizationVariant collisionSecond;
    collisionSecond.path = "mode.vti";
    collisionSecond.selectors = {{"a", "x" + unitSeparator + "b=p"}, {"b", "q"}};
    collision.variants.push_back(collisionSecond);
    try {
        manifest::ValidateVisualizationCatalog(collision, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("Distinct selector maps were treated as the same coordinate: ")
                    + error.what());
    }
    collision.variants.back() = collision.variants.front();
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(collision, output.path); },
            "duplicate selector coordinates")) {
        return Fail("A genuinely duplicate selector coordinate was accepted.");
    }

    manifest::VisualizationCatalog invalidReaderTime = validCatalog;
    invalidReaderTime.variants.front().hasReaderTime = true;
    invalidReaderTime.variants.front().readerTime =
        std::numeric_limits<double>::quiet_NaN();
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidReaderTime, output.path); },
            "readerTime must be finite")) {
        return Fail("A non-finite readerTime was accepted.");
    }
    invalidReaderTime.variants.front().readerTime =
        std::numeric_limits<double>::infinity();
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidReaderTime, output.path); },
            "readerTime must be finite")) {
        return Fail("An infinite readerTime was accepted.");
    }

    manifest::VisualizationCatalog symbolicNumeric = validCatalog;
    symbolicNumeric.axes.front().values = {{"late", "Late"}};
    symbolicNumeric.axes.front().defaultValue = "late";
    symbolicNumeric.variants.front().selectors["rank"] = "late";
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(symbolicNumeric, output.path); },
            "require numeric_value")) {
        return Fail("A symbolic numeric-axis key without numeric_value was accepted.");
    }
    symbolicNumeric.axes.front().values = {{"2.5", "2.5"}};
    symbolicNumeric.axes.front().defaultValue = "2.5";
    symbolicNumeric.variants.front().selectors["rank"] = "2.5";
    try {
        manifest::ValidateVisualizationCatalog(symbolicNumeric, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A finite numeric key without redundant numeric_value was rejected: ")
                    + error.what());
    }
    if (!manifest::VisualizationKeyIsFiniteNumber("1e3")) {
        return Fail("A finite exponent-form numeric selector key was rejected.");
    }
    for (const std::string& invalidNumericKey : {
             "nan", "inf", " 1", "1 ", "1x", "0x1p2",
         }) {
        if (manifest::VisualizationKeyIsFiniteNumber(invalidNumericKey)) {
            return Fail("A non-finite or non-decimal numeric selector key was accepted: "
                        + invalidNumericKey);
        }
    }

    manifest::VisualizationCatalog invalidMapping = validCatalog;
    invalidMapping.variants.front().quantities.front().arrayName.clear();
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "arrayName must be non-empty")) {
        return Fail("An empty variant quantity array name was accepted.");
    }
    invalidMapping = validCatalog;
    invalidMapping.variants.front().quantities.front().arrayName = " \t";
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "arrayName must be non-empty")) {
        return Fail("A whitespace-only variant quantity array name was accepted.");
    }
    invalidMapping = validCatalog;
    invalidMapping.variants.front().quantities.front().arrayName = "field ";
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "arrayName must be non-empty")) {
        return Fail("A variant quantity array name with boundary whitespace was accepted.");
    }
    invalidMapping = validCatalog;
    invalidMapping.variants.front().quantities.front().arrayName =
        std::string("field\0ignored", 13);
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "embedded NUL")) {
        return Fail("A variant quantity array name with an embedded NUL was accepted.");
    }
    invalidMapping = validCatalog;
    invalidMapping.quantities.front().arrayName = std::string("field\0ignored", 13);
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "embedded NUL")) {
        return Fail("A default quantity array name with an embedded NUL was accepted.");
    }
    invalidMapping = validCatalog;
    invalidMapping.variants.front().quantities.push_back(
        invalidMapping.variants.front().quantities.front());
    if (!ThrowsContaining(
            [&] { manifest::ValidateVisualizationCatalog(invalidMapping, output.path); },
            "maps a quantity more than once")) {
        return Fail("A duplicate variant quantity mapping was accepted.");
    }

    TemporaryDirectory outside("outside");
    const std::filesystem::path outsideFile = outside.path / "outside.vti";
    {
        std::ofstream data(outsideFile);
        data << "outside";
    }
    const std::filesystem::path linkPath = output.path / "outside-link.vti";
    std::error_code linkError;
    std::filesystem::create_symlink(outsideFile, linkPath, linkError);
    if (!linkError) {
        manifest::VisualizationCatalog escapedLink = validCatalog;
        escapedLink.variants.front().path = "outside-link.vti";
        if (!ThrowsContaining(
                [&] { manifest::ValidateVisualizationCatalog(escapedLink, output.path); },
                "escapes the output directory")) {
            return Fail("A variant symlink targeting a file outside the output directory was accepted.");
        }
    } else {
        std::cerr << "Skipping external symlink containment test: "
                  << linkError.message() << "\n";
    }

    const std::filesystem::path internalLinkPath = output.path / "internal-link.vti";
    linkError.clear();
    std::filesystem::create_symlink(output.path / "mode.vti", internalLinkPath, linkError);
    if (!linkError) {
        manifest::VisualizationCatalog internalLink = validCatalog;
        internalLink.variants.front().path = "internal-link.vti";
        try {
            manifest::ValidateVisualizationCatalog(internalLink, output.path);
        } catch (const std::exception& error) {
            return Fail(std::string("A symlink to a file inside the output directory was rejected: ")
                        + error.what());
        }
    } else {
        std::cerr << "Skipping internal symlink containment test: "
                  << linkError.message() << "\n";
    }

    const std::string unicodeFileName =
        "\xe7\xbb\x93\xe6\x9e\x9c-\xf0\x9f\x98\x80.vti";
    {
        std::ofstream unicodeData(output.path / manifest::VisualizationPathFromUtf8(unicodeFileName));
        unicodeData << "unicode";
    }
    manifest::VisualizationCatalog unicodePath = validCatalog;
    unicodePath.variants.front().path = unicodeFileName;
    try {
        manifest::ValidateVisualizationCatalog(unicodePath, output.path);
    } catch (const std::exception& error) {
        return Fail(std::string("A valid UTF-8 variant path was rejected: ") + error.what());
    }
    std::ostringstream unicodeJson;
    manifest::WriteVisualizationCatalog(unicodeJson, unicodePath, "");
    if (unicodeJson.str().find(unicodeFileName) == std::string::npos) {
        return Fail("A UTF-8 variant path did not round-trip through JSON serialization.");
    }

    manifest::VisualizationCatalog localized = validCatalog;
    localized.axes.front().values.front().numericValue = 1.5;
    localized.variants.front().hasReaderTime = true;
    localized.variants.front().readerTime = 2.5;
    localized.variants.front().stepIndex = 3;
    std::ostringstream hostileStream;
    hostileStream.imbue(std::locale(std::locale::classic(), new NonJsonNumberPunctuation));
    hostileStream << std::showpos << std::hexfloat;
    hostileStream.precision(4);
    const std::ios::fmtflags originalFlags = hostileStream.flags();
    const std::streamsize originalPrecision = hostileStream.precision();
    manifest::WriteVisualizationCatalog(hostileStream, localized, "");
    const std::string localizedPayload = hostileStream.str();
    if (localizedPayload.find("\"numeric_value\": 1.5") == std::string::npos
        || localizedPayload.find("\"reader_time\": 2.5") == std::string::npos
        || localizedPayload.find("\"step_index\": 3") == std::string::npos) {
        return Fail("JSON numbers inherited non-JSON locale or stream formatting.");
    }
    if (hostileStream.flags() != originalFlags
        || hostileStream.precision() != originalPrecision) {
        return Fail("Writing a visualization catalog changed caller stream formatting state.");
    }

    std::string localizedRunId;
    {
        ScopedGlobalLocale hostileGlobal(
            std::locale(std::locale::classic(), new NonJsonNumberPunctuation));
        localizedRunId = manifest::MakeVisualizationRunId();
    }
    if (localizedRunId.find('_') != std::string::npos
        || localizedRunId.find_first_not_of("0123456789-") != std::string::npos) {
        return Fail("Visualization run IDs inherited global numeric punctuation.");
    }

    return 0;
}
