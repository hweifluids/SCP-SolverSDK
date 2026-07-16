#pragma once

#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkHDFReader.h>
#include <vtkInformation.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace streamcenterplus::vtkhdf {

inline constexpr const char* kStepMarker = "|vtkhdf_step=";

inline std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline bool IsVtkHdfExtension(const std::string& ext) {
    return ToLowerAscii(ext) == ".vtkhdf";
}

inline bool ParseStepToken(const std::filesystem::path& tokenPath,
                           std::filesystem::path* realPath,
                           vtkIdType* step) {
    const std::string token = tokenPath.string();
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
    if (realPath != nullptr) {
        *realPath = token.substr(0, marker);
    }
    if (step != nullptr) {
        const std::string stepText = token.substr(marker + std::char_traits<char>::length(kStepMarker));
        std::size_t parsedChars = 0;
        const long long parsedStep = std::stoll(stepText, &parsedChars);
        if (parsedChars != stepText.size() || parsedStep < 0) {
            throw std::runtime_error("Invalid VTKHDF time-step token: " + token);
        }
        *step = static_cast<vtkIdType>(parsedStep);
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
    std::string token = filePath.string();
    token += kStepMarker;
    std::string stepText = std::to_string(step);
    if (stepText.size() < 12) {
        token.append(12 - stepText.size(), '0');
    }
    token += stepText;
    return std::filesystem::path(token);
}

inline std::vector<std::filesystem::path> ExpandTimeSteps(const std::filesystem::path& filePath) {
    const std::filesystem::path realPath = RealPath(filePath);
    if (!IsVtkHdfExtension(realPath.extension().string())) {
        return {filePath};
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    reader->SetFileName(realPath.string().c_str());
    reader->UpdateInformation();
    const vtkIdType numberOfSteps = reader->GetHasTransientData()
        ? std::max<vtkIdType>(reader->GetNumberOfSteps(), 1)
        : 1;
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
        throw std::runtime_error("Input is not a .vtkhdf path: " + tokenPath.string());
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    reader->SetFileName(realPath.string().c_str());
    reader->UpdateInformation();
    if (step >= 0) {
        reader->SetStep(step);
    }
    reader->Update();
    vtkDataObject* object = reader->GetOutputDataObject(0);
    if (object == nullptr) {
        throw std::runtime_error("Unsupported or unreadable VTKHDF dataset: " + tokenPath.string());
    }
    return object;
}

inline vtkSmartPointer<vtkDataSet> ReadDataSet(const std::filesystem::path& tokenPath) {
    vtkSmartPointer<vtkDataObject> object = ReadDataObject(tokenPath);
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(object);
    if (dataSet == nullptr) {
        throw std::runtime_error("The VTKHDF file is not a vtkDataSet: " + tokenPath.string());
    }
    return dataSet;
}

inline std::vector<double> TimeValues(const std::filesystem::path& filePath) {
    const std::filesystem::path realPath = RealPath(filePath);
    if (!IsVtkHdfExtension(realPath.extension().string())) {
        return {};
    }

    auto reader = vtkSmartPointer<vtkHDFReader>::New();
    reader->SetFileName(realPath.string().c_str());
    reader->UpdateInformation();
    const vtkIdType numberOfSteps = reader->GetHasTransientData()
        ? std::max<vtkIdType>(reader->GetNumberOfSteps(), 1)
        : 1;
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
        reader->UpdateInformation();
        values.push_back(reader->GetTimeValue());
    }
    return values;
}

}  // namespace streamcenterplus::vtkhdf
