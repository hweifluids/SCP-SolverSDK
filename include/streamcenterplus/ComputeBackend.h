#pragma once

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace streamcenterplus {

enum class ComputeBackend {
    Cpu,
    Cuda,
};

inline std::string NormalizeComputeBackendName(std::string value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline ComputeBackend ParseComputeBackend(const std::string& rawValue) {
    const std::string value = NormalizeComputeBackendName(rawValue);
    if (value == "cpu" || value == "cpp" || value == "eigen" || value == "native") {
        return ComputeBackend::Cpu;
    }
    if (value == "cuda" || value == "gpu") {
        return ComputeBackend::Cuda;
    }
    throw std::runtime_error("Unsupported compute_backend: " + rawValue + ". Expected cpu or cuda.");
}

inline const char* ComputeBackendName(ComputeBackend backend) noexcept {
    return backend == ComputeBackend::Cuda ? "cuda" : "cpu";
}

inline bool CudaBackendCompiled() noexcept {
#if defined(STREAMCENTERPLUS_HAS_CUDA) && STREAMCENTERPLUS_HAS_CUDA
    return true;
#else
    return false;
#endif
}

inline void RequireCudaBackend(const std::string& solverName) {
    if (!CudaBackendCompiled()) {
        throw std::runtime_error(
            solverName +
            " requested compute_backend=cuda, but this solver library was built without CUDA support. "
            "Install a supported CUDA Toolkit and rebuild, or set compute_backend=cpu in the deck.");
    }
}

}  // namespace streamcenterplus
