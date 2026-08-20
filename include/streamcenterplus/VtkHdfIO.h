#pragma once

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkErrorCode.h>
#include <vtkExecutive.h>
#include <vtkHDFReader.h>
#include <vtkInformation.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace streamcenterplus::vtkhdf {

inline constexpr const char* kStepMarker = "|vtkhdf_step=";

inline std::string PathToUtf8(const std::filesystem::path& path) {
    const auto encoded = path.generic_u8string();
    std::string result;
    result.reserve(encoded.size());
    for (const auto ch : encoded) {
        result.push_back(static_cast<char>(ch));
    }
    return result;
}

inline std::filesystem::path PathFromUtf8(const std::string& value) {
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

inline std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline bool IsVtkHdfExtension(const std::string& ext) {
    return ToLowerAscii(ext) == ".vtkhdf";
}

inline vtkSmartPointer<vtkCallbackCommand> ObserveReaderErrors(vtkObject* reader,
                                                                bool* hadError) {
    auto observer = vtkSmartPointer<vtkCallbackCommand>::New();
    observer->SetClientData(hadError);
    observer->SetCallback([](vtkObject*, unsigned long, void* clientData, void*) {
        *static_cast<bool*>(clientData) = true;
    });
    reader->AddObserver(vtkCommand::ErrorEvent, observer);
    return observer;
}

inline bool ParseStepToken(const std::filesystem::path& tokenPath,
                           std::filesystem::path* realPath,
                           vtkIdType* step) {
    const std::string token = PathToUtf8(tokenPath);
    const std::size_t marker = token.rfind(kStepMarker);
    if (marker == std::string::npos) {
        if (realPath != nullptr) {
            *realPath = tokenPath;
        }
        if (step != nullptr) {
            *step = -1;
        }
        return false;
    }
#if !defined(_WIN32)
    std::error_code statusError;
    const std::filesystem::file_status nativePathStatus =
        std::filesystem::symlink_status(tokenPath, statusError);
    if (statusError) {
        throw std::runtime_error("Cannot inspect VTKHDF path: " + token + ": "
                                 + statusError.message());
    }
    if (std::filesystem::exists(nativePathStatus)) {
        if (realPath != nullptr) {
            *realPath = tokenPath;
        }
        if (step != nullptr) {
            *step = -1;
        }
        return false;
    }
#endif
    const std::string realPathText = token.substr(0, marker);
    const std::string stepText =
        token.substr(marker + std::char_traits<char>::length(kStepMarker));
    if (realPathText.empty() || stepText.empty()
        || !std::all_of(stepText.begin(), stepText.end(), [](unsigned char ch) {
               return ch >= '0' && ch <= '9';
           })) {
        throw std::runtime_error("Invalid VTKHDF time-step token: " + token);
    }
    vtkIdType parsedStep = -1;
    try {
        std::size_t parsedChars = 0;
        const unsigned long long parsed = std::stoull(stepText, &parsedChars);
        if (parsedChars != stepText.size()
            || parsed > static_cast<unsigned long long>(std::numeric_limits<vtkIdType>::max())) {
            throw std::out_of_range("VTKHDF step is out of range");
        }
        parsedStep = static_cast<vtkIdType>(parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid VTKHDF time-step token: " + token);
    }
    if (realPath != nullptr) {
        *realPath = PathFromUtf8(realPathText);
    }
    if (step != nullptr) {
        *step = parsedStep;
    }
    return true;
}

inline std::filesystem::path RealPath(const std::filesystem::path& tokenPath) {
    std::filesystem::path realPath;
    ParseStepToken(tokenPath, &realPath, nullptr);
    return realPath;
}

inline std::string Extension(const std::filesystem::path& tokenPath) {
    return ToLowerAscii(RealPath(tokenPath).extension().string());
}

inline bool IsVtkHdfPath(const std::filesystem::path& tokenPath) {
    return IsVtkHdfExtension(Extension(tokenPath));
}

inline std::filesystem::path MakeStepToken(const std::filesystem::path& filePath, vtkIdType step) {
    if (step < 0) {
        throw std::runtime_error("VTKHDF time-step tokens require a non-negative step.");
    }
    std::string token = PathToUtf8(filePath);
    token += kStepMarker;
    std::string stepText = std::to_string(step);
    if (stepText.size() < 12) {
        token.append(12 - stepText.size(), '0');
    }
    token += stepText;
    return PathFromUtf8(token);
}

inline std::vector<std::filesystem::path> ExpandTimeSteps(const std::filesystem::path& filePath) {
    const std::filesystem::path realPath = RealPath(filePath);
    if (!IsVtkHdfExtension(realPath.extension().string())) {
        return {filePath};
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    bool hadError = false;
    [[maybe_unused]] const auto errorObserver = ObserveReaderErrors(reader, &hadError);
    [[maybe_unused]] const auto executiveErrorObserver =
        ObserveReaderErrors(reader->GetExecutive(), &hadError);
    const std::string fileName = PathToUtf8(realPath);
    if (!reader->CanReadFile(fileName.c_str())) {
        throw std::runtime_error("Unsupported or unreadable VTKHDF dataset: "
                                 + PathToUtf8(realPath));
    }
    reader->SetFileName(fileName.c_str());
    reader->UpdateInformation();
    if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError) {
        throw std::runtime_error("Cannot read VTKHDF metadata: " + PathToUtf8(realPath));
    }
    const vtkIdType numberOfSteps = reader->GetNumberOfSteps();
    if (numberOfSteps < 1) {
        throw std::runtime_error("VTKHDF dataset did not report any readable steps: "
                                 + PathToUtf8(realPath));
    }
    if (numberOfSteps <= 1) {
        return {realPath};
    }

    std::vector<std::filesystem::path> frames;
    frames.reserve(static_cast<std::size_t>(numberOfSteps));
    for (vtkIdType step = 0; step < numberOfSteps; ++step) {
        frames.push_back(MakeStepToken(realPath, step));
    }
    return frames;
}

inline vtkSmartPointer<vtkDataObject> ReadDataObject(const std::filesystem::path& tokenPath) {
    std::filesystem::path realPath;
    vtkIdType step = -1;
    ParseStepToken(tokenPath, &realPath, &step);
    if (!IsVtkHdfExtension(realPath.extension().string())) {
        throw std::runtime_error("Input is not a .vtkhdf path: " + PathToUtf8(tokenPath));
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    bool hadError = false;
    [[maybe_unused]] const auto errorObserver = ObserveReaderErrors(reader, &hadError);
    [[maybe_unused]] const auto executiveErrorObserver =
        ObserveReaderErrors(reader->GetExecutive(), &hadError);
    const std::string fileName = PathToUtf8(realPath);
    if (!reader->CanReadFile(fileName.c_str())) {
        throw std::runtime_error("Unsupported or unreadable VTKHDF dataset: "
                                 + PathToUtf8(tokenPath));
    }
    reader->SetFileName(fileName.c_str());
    reader->UpdateInformation();
    if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError) {
        throw std::runtime_error("Cannot read VTKHDF metadata: " + PathToUtf8(tokenPath));
    }
    if (step >= 0) {
        if (step >= reader->GetNumberOfSteps()) {
            throw std::runtime_error("VTKHDF time-step token is out of range: "
                                     + PathToUtf8(tokenPath));
        }
        reader->SetStep(step);
    }
    reader->Update();
    vtkDataObject* object = reader->GetOutputDataObject(0);
    if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError || object == nullptr) {
        throw std::runtime_error("Unsupported or unreadable VTKHDF dataset: "
                                 + PathToUtf8(tokenPath));
    }
    return object;
}

inline vtkSmartPointer<vtkDataSet> ReadDataSet(const std::filesystem::path& tokenPath) {
    vtkSmartPointer<vtkDataObject> object = ReadDataObject(tokenPath);
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(object);
    if (dataSet == nullptr) {
        throw std::runtime_error("The VTKHDF file is not a vtkDataSet: "
                                 + PathToUtf8(tokenPath));
    }
    return dataSet;
}

inline std::vector<double> TimeValues(const std::filesystem::path& filePath) {
    const std::filesystem::path realPath = RealPath(filePath);
    if (!IsVtkHdfExtension(realPath.extension().string())) {
        return {};
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    bool hadError = false;
    [[maybe_unused]] const auto errorObserver = ObserveReaderErrors(reader, &hadError);
    [[maybe_unused]] const auto executiveErrorObserver =
        ObserveReaderErrors(reader->GetExecutive(), &hadError);
    const std::string fileName = PathToUtf8(realPath);
    if (!reader->CanReadFile(fileName.c_str())) {
        throw std::runtime_error("Unsupported or unreadable VTKHDF dataset: "
                                 + PathToUtf8(realPath));
    }
    reader->SetFileName(fileName.c_str());
    reader->UpdateInformation();
    if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError) {
        throw std::runtime_error("Cannot read VTKHDF metadata: " + PathToUtf8(realPath));
    }
    const vtkIdType numberOfSteps = reader->GetNumberOfSteps();
    if (numberOfSteps < 1) {
        throw std::runtime_error("VTKHDF dataset did not report any readable steps: "
                                 + PathToUtf8(realPath));
    }
    if (vtkInformation* info = reader->GetOutputInformation(0)) {
        vtkInformationDoubleVectorKey* timeStepsKey = vtkStreamingDemandDrivenPipeline::TIME_STEPS();
        if (info->Has(timeStepsKey)) {
            const int count = info->Length(timeStepsKey);
            if (count > 0) {
                std::vector<double> values(static_cast<std::size_t>(count), 0.0);
                info->Get(timeStepsKey, values.data());
                return values;
            }
        }
    }

    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(numberOfSteps));
    for (vtkIdType step = 0; step < numberOfSteps; ++step) {
        reader->SetStep(step);
        reader->Update();
        if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError) {
            throw std::runtime_error("Cannot read VTKHDF time step: "
                                     + PathToUtf8(MakeStepToken(realPath, step)));
        }
        values.push_back(reader->GetTimeValue());
    }
    return values;
}

}  // namespace streamcenterplus::vtkhdf
