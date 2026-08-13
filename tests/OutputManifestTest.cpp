#include <streamcenterplus/OutputManifest.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

void WriteFixtureFile(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << "fixture";
}

std::string XmlAttributeEscape(const std::string& value) {
    std::string escaped;
    for (const char ch : value) {
        switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

void WriteSingleDataSetPvd(const std::filesystem::path& path,
                           const std::string& referencedFile) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream pvd(path, std::ios::binary);
    pvd << "<VTKFile type=\"Collection\"><Collection>"
           "<DataSet file=\""
        << XmlAttributeEscape(referencedFile)
        << "\"/></Collection></VTKFile>";
}

bool CreateDirectoryLink(const std::filesystem::path& target,
                         const std::filesystem::path& link) {
#ifdef _WIN32
    constexpr DWORD allowUnprivilegedCreate = 0x2u;
    if (CreateSymbolicLinkW(link.c_str(),
                            target.c_str(),
                            SYMBOLIC_LINK_FLAG_DIRECTORY | allowUnprivilegedCreate)
        || CreateSymbolicLinkW(
            link.c_str(), target.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY)) {
        return true;
    }

    std::wstring command = L"cmd.exe /d /c mklink /J \"";
    command += link.native();
    command += L"\" \"";
    command += target.native();
    command += L"\"";
    std::vector<wchar_t> commandLine(command.begin(), command.end());
    commandLine.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr,
                        commandLine.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_NO_WINDOW,
                        nullptr,
                        nullptr,
                        &startup,
                        &process)) {
        return false;
    }
    const DWORD waitResult = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    const bool exited = waitResult == WAIT_OBJECT_0
        && GetExitCodeProcess(process.hProcess, &exitCode) != FALSE;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exited && exitCode == 0;
#else
    std::error_code error;
    std::filesystem::create_directory_symlink(target, link, error);
    return !error;
#endif
}

}  // namespace

int main() {
    std::string controlThenHex(1, '\x01');
    controlThenHex += "ABC";
    if (manifest::JsonEscape(controlThenHex) != "\\u0001ABC") {
        return Fail("Output-manifest JSON escaping did not use a fixed-width Unicode escape.");
    }

    TemporaryDirectory pvdDirectory("pvd");
    const std::filesystem::path pvdPath =
        pvdDirectory.path / "collections" / "collection.pvd";
    WriteFixtureFile(pvdDirectory.path / "collections" / "frames" / "a&b.vti");
    WriteFixtureFile(pvdDirectory.path / "collections" / "frames" / "c.vti");
    WriteFixtureFile(pvdDirectory.path / "collections" / "frames" / "d.vti");
    {
        std::filesystem::create_directories(pvdPath.parent_path());
        std::ofstream pvd(pvdPath, std::ios::binary);
        pvd << "<?xml version='1.0'?>\n"
               "<VTKFile type='Collection' version='0.1'>\n"
               "  <Collection>\n"
               "    <DataSet\n"
               "      timestep='1.5'\n"
               "      file='frames/./a&amp;b.vti'/>\n"
               "    <DataSet timestep=\"2\" file=\"frames/c.vti\"/>"
               "<DataSet timestep=\"3\" file=\"frames/d.vti\"/>\n"
               "  </Collection>\n"
               "</VTKFile>\n";
    }
    std::ostringstream pvdJson;
    manifest::WritePvdFile(pvdJson, pvdPath, pvdDirectory.path, "");
    const std::string pvdPayload = pvdJson.str();
    if (CountOccurrences(pvdPayload, "\"file\":") != 3
        || pvdPayload.find("\"collections/frames/a&b.vti\"") == std::string::npos
        || pvdPayload.find("frames/./a&b.vti") != std::string::npos) {
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

    const std::filesystem::path absoluteReferencePvd =
        pvdDirectory.path / "absolute-reference.pvd";
    WriteSingleDataSetPvd(
        absoluteReferencePvd,
        manifest::VisualizationPathToUtf8(
            std::filesystem::absolute(
                pvdDirectory.path / "collections" / "frames" / "c.vti")));
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(
                    ignored, absoluteReferencePvd, pvdDirectory.path, "");
            },
            "must be relative")) {
        return Fail("A PVD DataSet absolute file path was accepted.");
    }

    TemporaryDirectory outside("outside");
    const std::filesystem::path outsideFile = outside.path / "outside.vti";
    WriteFixtureFile(outsideFile);
    const std::filesystem::path parentEscapePvd =
        pvdDirectory.path / "parent-escape.pvd";
    WriteSingleDataSetPvd(
        parentEscapePvd,
        manifest::VisualizationPathToUtf8(
            std::filesystem::relative(outsideFile, pvdDirectory.path)));
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(
                    ignored, parentEscapePvd, pvdDirectory.path, "");
            },
            "escapes the output directory")) {
        return Fail("A PVD DataSet parent traversal outside the output directory was accepted.");
    }

    const std::filesystem::path missingReferencePvd =
        pvdDirectory.path / "missing-reference.pvd";
    WriteSingleDataSetPvd(missingReferencePvd, "missing/frame.vti");
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(
                    ignored, missingReferencePvd, pvdDirectory.path, "");
            },
            "does not exist")) {
        return Fail("A PVD DataSet missing file was accepted.");
    }

    const std::filesystem::path directoryReference =
        pvdDirectory.path / "directory-reference.vti";
    std::filesystem::create_directories(directoryReference);
    const std::filesystem::path directoryReferencePvd =
        pvdDirectory.path / "directory-reference.pvd";
    WriteSingleDataSetPvd(directoryReferencePvd, "directory-reference.vti");
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(
                    ignored, directoryReferencePvd, pvdDirectory.path, "");
            },
            "regular file")) {
        return Fail("A PVD DataSet directory reference was accepted as a file.");
    }

    const std::filesystem::path escapingLink =
        pvdDirectory.path / "escaping-link";
    if (!CreateDirectoryLink(outside.path, escapingLink)) {
        return Fail("Could not create the directory link needed by the PVD escape test.");
    }
    const std::filesystem::path linkEscapePvd =
        pvdDirectory.path / "symlink-escape.pvd";
    WriteSingleDataSetPvd(linkEscapePvd, "escaping-link/outside.vti");
    if (!ThrowsContaining(
            [&] {
                std::ostringstream ignored;
                manifest::WritePvdFile(
                    ignored, linkEscapePvd, pvdDirectory.path, "");
            },
            "escapes the output directory")) {
        return Fail("A PVD DataSet directory link escaping the output directory was accepted.");
    }

    const std::filesystem::path canonicalTarget =
        pvdDirectory.path / "canonical" / "target.vti";
    WriteFixtureFile(canonicalTarget);
    const std::filesystem::path safeLink =
        pvdDirectory.path / "safe-link";
    if (!CreateDirectoryLink(canonicalTarget.parent_path(), safeLink)) {
        return Fail("Could not create the second directory link needed by the PVD safety test.");
    }
    const std::filesystem::path safeLinkPvd =
        pvdDirectory.path / "safe-symlink.pvd";
    WriteSingleDataSetPvd(safeLinkPvd, "safe-link/target.vti");
    std::ostringstream safeLinkJson;
    manifest::WritePvdFile(
        safeLinkJson, safeLinkPvd, pvdDirectory.path, "");
    if (safeLinkJson.str().find("\"canonical/target.vti\"")
        == std::string::npos) {
        return Fail("A contained PVD DataSet directory link was not published as its canonical path.");
    }

    TemporaryDirectory integratedPvdDirectory("integrated-pvd");
    const std::filesystem::path integratedDataPath =
        integratedPvdDirectory.path / "collections" / "frames" / "frame.vtkhdf";
    WriteFixtureFile(integratedDataPath);
    WriteSingleDataSetPvd(
        integratedPvdDirectory.path / "collections" / "series.pvd",
        "frames/./frame.vtkhdf");
    manifest::WriteOutputManifest(integratedPvdDirectory.path, "DFT");
    std::ifstream integratedManifest(
        integratedPvdDirectory.path / "output_manifest.json", std::ios::binary);
    if (!integratedManifest) {
        return Fail("WriteOutputManifest did not create its manifest file.");
    }
    std::ostringstream integratedManifestBuffer;
    integratedManifestBuffer << integratedManifest.rdbuf();
    const std::string integratedManifestPayload = integratedManifestBuffer.str();
    if (integratedManifestPayload.find(
               "\"file\": \"collections/frames/frame.vtkhdf\"")
            == std::string::npos
        || integratedManifestPayload.find("frames/./frame.vtkhdf")
            != std::string::npos) {
        return Fail("WriteOutputManifest did not publish the safe canonical PVD DataSet path.");
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

    if (!ThrowsContaining(
            [&] { manifest::RelativePath(outsideFile, pvdDirectory.path); },
            "escapes the output directory")) {
        return Fail("A path outside the output directory was serialized as an inventory path.");
    }

    return 0;
}
