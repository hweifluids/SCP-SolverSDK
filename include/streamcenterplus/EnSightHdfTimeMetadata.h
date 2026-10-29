#pragma once

#include <streamcenterplus/EnSightTimePolicy.h>
#include <streamcenterplus/VtkHdfIO.h>
#include <vtk_hdf5.h>

#include <filesystem>
#include <optional>
#include <stdexcept>

namespace streamcenterplus::vtkhdf {

// Opt-in metadata helper: consumers link VTK::hdf5 explicitly. Reading this
// scalar attribute never loads mesh/field arrays or rewrites Steps/Values.
inline std::optional<double> EnSightTimeQuantum(const std::filesystem::path& path) {
    struct Handle {
        hid_t id = -1;
        herr_t (*close)(hid_t) = nullptr;
        ~Handle() { if (id >= 0 && close != nullptr) { close(id); } }
    };
    const Handle file{H5Fopen(PathToUtf8(RealPath(path)).c_str(), H5F_ACC_RDONLY, H5P_DEFAULT),
                      H5Fclose};
    if (file.id < 0) {
        throw std::runtime_error("Cannot inspect VTKHDF EnSight time metadata: " + path.string());
    }
    const Handle group{H5Gopen2(file.id, "/VTKHDF", H5P_DEFAULT), H5Gclose};
    if (group.id < 0) {
        throw std::runtime_error("Missing VTKHDF root group: " + path.string());
    }
    const htri_t present = H5Aexists(group.id, ensight_time::kTimeQuantumAttribute);
    if (present == 0) {
        return std::nullopt;
    }
    if (present < 0) {
        throw std::runtime_error("Cannot inspect VTKHDF EnSight time quantum: " + path.string());
    }
    const Handle attribute{
        H5Aopen(group.id, ensight_time::kTimeQuantumAttribute, H5P_DEFAULT), H5Aclose};
    const Handle space{attribute.id < 0 ? -1 : H5Aget_space(attribute.id), H5Sclose};
    const Handle type{attribute.id < 0 ? -1 : H5Aget_type(attribute.id), H5Tclose};
    double quantum = 0.0;
    if (attribute.id < 0 || space.id < 0 || type.id < 0 ||
        H5Sget_simple_extent_type(space.id) != H5S_SCALAR ||
        H5Tget_class(type.id) != H5T_FLOAT || H5Tget_size(type.id) != sizeof(double) ||
        H5Tget_precision(type.id) != 64 ||
        H5Aread(attribute.id, H5T_NATIVE_DOUBLE, &quantum) < 0 ||
        !std::isfinite(quantum) || quantum <= 0.0) {
        throw std::runtime_error("Invalid VTKHDF EnSight time quantum: " + path.string());
    }
    return quantum;
}

}  // namespace streamcenterplus::vtkhdf
