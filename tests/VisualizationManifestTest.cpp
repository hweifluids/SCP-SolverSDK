#include <streamcenterplus/VisualizationManifest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int Fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

}  // namespace

int main() {
    namespace manifest = streamcenterplus::manifest;

    if (manifest::CanonicalSolverFamily("fdSPOD_RAW") != "fdspod") {
        return Fail("fdSPOD_RAW did not normalize to fdspod.");
    }
    if (manifest::SolverVariantFromName("TOD_RAW") != "raw") {
        return Fail("TOD_RAW did not normalize to the raw variant.");
    }

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("scp-visualization-manifest-" + manifest::MakeVisualizationRunId());
    std::filesystem::create_directories(root);
    const std::filesystem::path dataPath = root / "mode.vti";
    {
        std::ofstream data(dataPath);
        data << "fixture";
    }

    manifest::VisualizationCatalog catalog = manifest::MakeVisualizationCatalog("DFT");
    catalog.axes.push_back({
        "rank",
        "Rank",
        "numeric",
        "rank",
        "",
        true,
        "1",
        {{"1", "rank=1", true, 1.0}},
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

    try {
        manifest::ValidateVisualizationCatalog(catalog, root);
    } catch (const std::exception& error) {
        std::filesystem::remove_all(root);
        return Fail(std::string("A valid catalog was rejected: ") + error.what());
    }

    std::ostringstream json;
    manifest::WriteVisualizationCatalog(json, catalog, "  ");
    const std::string payload = json.str();
    if (payload.find("\"solver_family\": \"dft\"") == std::string::npos
        || payload.find("\"numeric_value\": 1") == std::string::npos
        || payload.find("\"quantity_id\": \"mode_magnitude\"") == std::string::npos) {
        std::filesystem::remove_all(root);
        return Fail("Serialized catalog omitted required semantic fields.");
    }

    catalog.variants.front().selectors["rank"] = "2";
    bool rejected = false;
    try {
        manifest::ValidateVisualizationCatalog(catalog, root);
    } catch (const std::exception&) {
        rejected = true;
    }
    std::filesystem::remove_all(root);
    return rejected ? 0 : Fail("An unknown numeric axis value was accepted.");
}
