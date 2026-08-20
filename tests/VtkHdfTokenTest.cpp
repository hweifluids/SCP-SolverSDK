#include <streamcenterplus/VtkHdfIO.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

int Fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

bool ThrowsRuntimeError(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

struct TemporaryDirectory {
    TemporaryDirectory()
        : path(std::filesystem::temp_directory_path()
               / ("scp-vtkhdf-token-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)))) {
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

}  // namespace

int main() {
    namespace vtkhdf = streamcenterplus::vtkhdf;

    const std::filesystem::path unicodePath = vtkhdf::PathFromUtf8(
        "\xe7\xbb\x93\xe6\x9e\x9c-\xf0\x9f\x98\x80.vtkhdf");
    const std::filesystem::path token = vtkhdf::MakeStepToken(unicodePath, 42);
    std::filesystem::path parsedPath;
    vtkIdType parsedStep = -1;
    if (!vtkhdf::ParseStepToken(token, &parsedPath, &parsedStep)
        || parsedPath != unicodePath || parsedStep != 42) {
        return Fail("A valid Unicode VTKHDF time-step token did not round-trip.");
    }
    if (vtkhdf::PathToUtf8(token).find("|vtkhdf_step=000000000042") == std::string::npos) {
        return Fail("A VTKHDF time-step token did not use the stable padded representation.");
    }

    const std::filesystem::path plain = "plain.vtkhdf";
    parsedPath.clear();
    parsedStep = 123;
    if (vtkhdf::ParseStepToken(plain, &parsedPath, &parsedStep)
        || parsedPath != plain || parsedStep != -1) {
        return Fail("A plain VTKHDF path was mistaken for a time-step token.");
    }

    for (const std::string& suffix : {"", "garbage", "-1", "1x", "+1"}) {
        const std::filesystem::path invalid =
            vtkhdf::PathFromUtf8("input.vtkhdf|vtkhdf_step=" + suffix);
        if (!ThrowsRuntimeError([&] { vtkhdf::ParseStepToken(invalid, nullptr, nullptr); })) {
            return Fail("An invalid VTKHDF token was accepted: " + vtkhdf::PathToUtf8(invalid));
        }
    }
    const std::filesystem::path tooLarge = vtkhdf::PathFromUtf8(
        "input.vtkhdf|vtkhdf_step=999999999999999999999999999999999999999999");
    parsedPath = "unchanged.vtkhdf";
    parsedStep = 7;
    if (!ThrowsRuntimeError(
            [&] { vtkhdf::ParseStepToken(tooLarge, &parsedPath, &parsedStep); })
        || parsedPath != std::filesystem::path("unchanged.vtkhdf") || parsedStep != 7) {
        return Fail("An out-of-range VTKHDF token modified outputs or was accepted.");
    }
    if (!ThrowsRuntimeError([&] { vtkhdf::MakeStepToken(plain, -1); })) {
        return Fail("MakeStepToken accepted a negative step.");
    }

    TemporaryDirectory temporary;
#if !defined(_WIN32)
    const std::filesystem::path markerNamedFile =
        temporary.path / "frame.vtkhdf|vtkhdf_step=000000000042";
    {
        std::ofstream output(markerNamedFile, std::ios::binary);
        output << "fixture";
    }
    parsedPath.clear();
    parsedStep = 123;
    if (vtkhdf::ParseStepToken(markerNamedFile, &parsedPath, &parsedStep)
        || parsedPath != markerNamedFile || parsedStep != -1) {
        return Fail("An existing POSIX path ending in the VTKHDF token marker was misparsed.");
    }
#endif

    const std::filesystem::path missing = temporary.path / "missing.vtkhdf";
    if (!ThrowsRuntimeError([&] { vtkhdf::ExpandTimeSteps(missing); })
        || !ThrowsRuntimeError([&] { vtkhdf::ReadDataObject(missing); })
        || !ThrowsRuntimeError([&] { vtkhdf::TimeValues(missing); })) {
        return Fail("A missing VTKHDF file was reported as a readable single-step dataset.");
    }

    const std::filesystem::path disguisedText = temporary.path / "text.vtkhdf";
    {
        std::ofstream output(disguisedText, std::ios::binary);
        output << "not an HDF5 file";
    }
    if (!ThrowsRuntimeError([&] { vtkhdf::ExpandTimeSteps(disguisedText); })
        || !ThrowsRuntimeError([&] { vtkhdf::ReadDataObject(disguisedText); })
        || !ThrowsRuntimeError([&] { vtkhdf::TimeValues(disguisedText); })) {
        return Fail("A text file with a .vtkhdf suffix was reported as readable.");
    }

    return 0;
}
