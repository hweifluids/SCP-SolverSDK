#include <streamcenterplus/OutputManifest.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace manifest = streamcenterplus::manifest;

namespace {

struct TemporaryDirectory {
    explicit TemporaryDirectory(const std::string& suffix)
        : path(std::filesystem::temp_directory_path()
               / ("scp-output-manifest-" + suffix + "-"
                  + manifest::MakeVisualizationRunId())) {
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

int Fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

bool ThrowsContaining(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::exception& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

std::size_t CountOccurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    for (std::size_t position = 0;
         (position = text.find(needle, position)) != std::string::npos;
         position += needle.size()) {
        ++count;
    }
    return count;
}

bool HasTemporaryManifest(const std::filesystem::path& directory) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const std::string name = manifest::VisualizationPathToUtf8(entry.path().filename());
        if (name.rfind("output_manifest.json.tmp-", 0) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    std::string controlThenHex(1, '\x01');
    controlThenHex += "ABC";
    if (manifest::JsonEscape(controlThenHex) != "\\u0001ABC") {
        return Fail("Output-manifest JSON escaping did not use a fixed-width Unicode escape.");
    }

    TemporaryDirectory pvdDirectory("pvd");
    const std::filesystem::path pvdPath = pvdDirectory.path / "collection.pvd";
    {
        std::ofstream pvd(pvdPath, std::ios::binary);
        pvd << "<?xml version='1.0'?>\n"
               "<VTKFile type='Collection' version='0.1'>\n"
               "  <Collection>\n"
               "    <DataSet\n"
               "      timestep='1.5'\n"
               "      file='a&amp;b.vti'/>\n"
               "    <DataSet timestep=\"2\" file=\"c.vti\"/>"
               "<DataSet timestep=\"3\" file=\"d.vti\"/>\n"
               "  </Collection>\n"
               "</VTKFile>\n";
    }
    std::ostringstream pvdJson;
    manifest::WritePvdFile(pvdJson, pvdPath, pvdDirectory.path, "");
    const std::string pvdPayload = pvdJson.str();
    if (CountOccurrences(pvdPayload, "\"file\":") != 3
        || pvdPayload.find("\"a&b.vti\"") == std::string::npos) {
        return Fail("A legal PVD with multiline, single-quoted, entity-encoded, or adjacent DataSet elements was misread.");
    }

    const std::filesystem::path wrongTypePvd = pvdDirectory.path / "wrong-type.pvd";
    {
        std::ofstream pvd(wrongTypePvd, std::ios::binary);
        pvd << "<VTKFile type='ImageData'><Collection>"
               "<DataSet file='x.vti'/></Collection></VTKFile>";
    }
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(ignored, wrongTypePvd, pvdDirectory.path, "");
            },
            "type must be Collection")) {
        return Fail("A VTK XML file whose type was not Collection was accepted as PVD.");
    }

    const std::filesystem::path invalidVti = pvdDirectory.path / "invalid.vti";
    {
        std::ofstream vtk(invalidVti, std::ios::binary);
        vtk << "<?xml version=\"1.0\"?>\n"
               "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
               "  <ImageData WholeExtent=\"0 1 0 1 0 0\" Origin=\"0 0 0\" Spacing=\"1 1 1\">\n"
               "    <Piece Extent=\"0 1 0 1 0 0\">\n"
               "      <PointData><DataArray type=\"Float64\" Name=\"bad\" format=\"ascii\">not-a-number</DataArray></PointData>\n"
               "      <CellData/>\n"
               "    </Piece>\n"
               "  </ImageData>\n"
               "</VTKFile>\n";
    }
    if (!ThrowsContaining(
            [&] { manifest::ReadDataObject(invalidVti); },
            "Unsupported or unreadable VTK dataset")) {
        return Fail("A VTI whose header was readable but whose array payload was invalid was accepted.");
    }

    TemporaryDirectory failedWrite("cleanup");
    {
        std::ofstream invalidPvd(failedWrite.path / "broken.pvd", std::ios::binary);
        invalidPvd << "not XML";
    }
    if (!ThrowsContaining(
            [&] { manifest::WriteOutputManifest(failedWrite.path, "DFT"); },
            "Cannot parse PVD collection")) {
        return Fail("A malformed PVD did not make output-manifest generation fail.");
    }
    if (HasTemporaryManifest(failedWrite.path)) {
        return Fail("Failed output-manifest generation left a temporary manifest behind.");
    }

    TemporaryDirectory outside("outside");
    const std::filesystem::path outsideFile = outside.path / "outside.vti";
    {
        std::ofstream data(outsideFile);
        data << "fixture";
    }
    if (!ThrowsContaining(
            [&] { manifest::RelativePath(outsideFile, pvdDirectory.path); },
            "escapes the output directory")) {
        return Fail("A path outside the output directory was serialized as an inventory path.");
    }

    return 0;
}
