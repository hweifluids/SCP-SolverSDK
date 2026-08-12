#pragma once

#include <streamcenterplus/VisualizationManifest.h>

#include <vtkAbstractArray.h>
#include <vtkCallbackCommand.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkErrorCode.h>
#include <vtkExecutive.h>
#include <vtkFieldData.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUtilities.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace streamcenterplus::manifest {

inline std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline std::string JsonEscape(const std::string& value) {
    return VisualizationJsonEscape(value);
}

inline std::string Quote(const std::string& value) {
    return "\"" + JsonEscape(value) + "\"";
}

inline bool HasSuffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

inline std::string PhysicalMeaning(const std::string& name, int components) {
    const std::string lower = ToLower(name);
    if (lower == "velocity") {
        return "Velocity vector field with components [u, v, w].";
    }
    if (lower == "pressure") {
        return "Pressure scalar field.";
    }
    if (lower == "mean_velocity") {
        return "Long-time mean velocity vector field with components [u, v, w].";
    }
    if (lower == "mean_velocity_magnitude") {
        return "Magnitude of the long-time mean velocity vector.";
    }
    if (lower == "mean_pressure") {
        return "Mean pressure scalar field.";
    }
    if (lower == "phi_velocity") {
        return "Real-valued spatial velocity mode vector field.";
    }
    if (lower == "phi_velocity_real") {
        return "Real part of a complex spatial velocity mode vector field.";
    }
    if (lower == "phi_velocity_imag") {
        return "Imaginary part of a complex spatial velocity mode vector field.";
    }
    if (lower == "phi_velocity_magnitude") {
        return "Magnitude of the spatial velocity mode.";
    }
    if (lower == "velocity_rom") {
        return "Reduced-order reconstructed velocity vector field.";
    }
    if (lower == "pressure_rom") {
        return "Reduced-order reconstructed pressure scalar field.";
    }
    if (lower == "velocity_rom_residual") {
        return "Velocity reconstruction residual vector field.";
    }
    if (lower == "pressure_rom_residual") {
        return "Pressure reconstruction residual scalar field.";
    }
    if (lower == "velocity_error") {
        return "Velocity reconstruction error vector field.";
    }
    if (lower == "velocity_error_magnitude") {
        return "Magnitude of the velocity reconstruction error.";
    }
    if (lower == "velocity_relative_error_magnitude") {
        return "Relative magnitude of the velocity reconstruction error.";
    }
    if (lower == "velocity_rom_magnitude") {
        return "Magnitude of the reduced-order reconstructed velocity.";
    }
    if (lower == "velocity_rom_divergence") {
        return "Divergence of the reduced-order reconstructed velocity.";
    }
    if (lower == "velocity_rom_vorticity") {
        return "Vorticity vector of the reduced-order reconstructed velocity.";
    }
    if (lower == "velocity_rom_vorticity_magnitude") {
        return "Magnitude of the vorticity of the reduced-order reconstructed velocity.";
    }
    if (lower == "velocity_rom_q_criterion") {
        return "Q-criterion computed from the reduced-order reconstructed velocity gradient.";
    }
    if (lower == "velocity_rom_lambda2") {
        return "Lambda-2 vortex criterion computed from the reduced-order reconstructed velocity gradient.";
    }
    if (lower == "weight") {
        return "Spatial integration weight.";
    }
    if (lower == "valid_mask") {
        return "Validity mask; nonzero values mark valid samples.";
    }
    if (lower == "mode_index") {
        return "One-based output mode index.";
    }
    if (lower == "frequency_index") {
        return "Frequency-bin index used by the solver.";
    }
    if (lower == "frequency") {
        return "Temporal frequency associated with the output mode.";
    }
    if (lower == "sigma") {
        return "Singular value or modal norm, depending on the solver.";
    }
    if (lower == "lambda" || lower == "lambda_real" || lower == "lambda_imag") {
        return "Modal eigenvalue or eigenvalue component.";
    }
    if (lower == "growth_rate") {
        return "DMD modal growth rate.";
    }
    if (lower == "energy") {
        return "Modal energy.";
    }
    if (lower == "energy_fraction") {
        return "Fraction of total retained modal energy.";
    }
    if (lower == "cumulative_energy_fraction") {
        return "Cumulative modal energy fraction up to this mode.";
    }
    if (lower == "amplitude") {
        return "Modal amplitude.";
    }
    if (lower == "frame_count") {
        return "Number of snapshots used to compute this output.";
    }
    if (lower == "weight_sum") {
        return "Sum of spatial integration weights.";
    }
    if (lower == "ftle_forward") {
        return "Forward finite-time Lyapunov exponent.";
    }
    if (lower == "ftle_backward") {
        return "Backward finite-time Lyapunov exponent.";
    }
    if (HasPrefix(lower, "fsle_")) {
        return "Finite-size Lyapunov exponent output or associated FSLE diagnostic.";
    }
    if (lower == "vort") {
        return components == 3 ? "Vorticity vector field." : "Vorticity-related scalar field.";
    }
    if (lower == "div") {
        return "Velocity divergence.";
    }
    if (lower == "qcr") {
        return "Q-criterion vortex diagnostic.";
    }
    if (lower == "delta") {
        return "Delta vortex criterion.";
    }
    if (lower == "lam2") {
        return "Lambda-2 vortex criterion.";
    }
    if (lower == "lci") {
        return "Lambda-ci swirling-strength criterion.";
    }
    if (lower == "omega" || lower == "oomega") {
        return "Omega vortex-identification criterion.";
    }
    if (lower == "hel" || lower == "nhel" || lower == "vrh" || lower == "ehd") {
        return "Helicity-based vortex diagnostic.";
    }
    if (lower == "okw") {
        return "Okubo-Weiss criterion.";
    }
    if (lower == "kvn") {
        return "Kinematic vorticity number.";
    }
    if (lower == "ivd") {
        return "Instantaneous vorticity deviation.";
    }
    if (HasPrefix(lower, "r_")) {
        return "Reynolds-stress tensor component.";
    }
    if (HasPrefix(lower, "am_")) {
        return "Mean-advection contribution to the momentum/pressure-gradient budget.";
    }
    if (HasPrefix(lower, "af_")) {
        return "Fluctuation/Reynolds-stress contribution to the momentum/pressure-gradient budget.";
    }
    if (HasPrefix(lower, "av_")) {
        return "Viscous contribution to the momentum/pressure-gradient budget.";
    }
    if (HasPrefix(lower, "g_")) {
        return "Total momentum/pressure-gradient budget contribution.";
    }
    if (HasPrefix(lower, "pressure_")) {
        return "Pressure-related scalar field derived by the solver.";
    }
    if (HasPrefix(lower, "cp_")) {
        return "Pressure-coefficient field.";
    }
    if (lower == "nu") {
        return "Kinematic viscosity field.";
    }
    if (lower.find("velocity") != std::string::npos && components == 3) {
        return "Velocity-related vector field with components [u, v, w].";
    }
    if (lower.find("pressure") != std::string::npos) {
        return "Pressure-related scalar field.";
    }
    if (HasSuffix(lower, "_magnitude") || HasSuffix(lower, "_mag")) {
        return "Magnitude of the named vector quantity.";
    }
    return "Solver output array; physical meaning is encoded by the variable name and solver documentation.";
}

inline std::string SemanticRole(const std::string& name, int components) {
    const std::string lower = ToLower(name);
    if (lower.find("velocity") != std::string::npos) {
        return components == 3 ? "velocity_vector" : "velocity_scalar";
    }
    if (lower.find("pressure") != std::string::npos) {
        return "pressure_scalar";
    }
    if (lower.find("phi_") == 0) {
        return "spatial_mode";
    }
    if (lower.find("mean_") == 0) {
        return "mean_field";
    }
    if (lower.find("ftle") == 0 || lower.find("fsle") == 0) {
        return "lagrangian_diagnostic";
    }
    if (components == 3) {
        return "vector_field";
    }
    return "scalar_or_metadata";
}

inline std::vector<std::string> ComponentLabels(const std::string& name, int components) {
    if (components == 3 && ToLower(name).find("velocity") != std::string::npos) {
        return {"u", "v", "w"};
    }
    if (components == 3) {
        return {"x", "y", "z"};
    }
    std::vector<std::string> labels;
    for (int i = 0; i < components; ++i) {
        labels.push_back("component_" + std::to_string(i));
    }
    return labels;
}

inline std::string RelativePath(const std::filesystem::path& path, const std::filesystem::path& root) {
    std::error_code ec;
    const std::filesystem::path relative = std::filesystem::relative(path, root, ec);
    if (ec || relative.empty() || *relative.begin() == "..") {
        throw std::runtime_error("Output manifest path escapes the output directory: "
                                 + VisualizationPathToUtf8(path));
    }
    return VisualizationPathToUtf8(relative);
}

inline vtkSmartPointer<vtkCallbackCommand> ObserveVtkReaderErrors(vtkObject* reader,
                                                                   bool* hadError) {
    auto observer = vtkSmartPointer<vtkCallbackCommand>::New();
    observer->SetClientData(hadError);
    observer->SetCallback([](vtkObject*, unsigned long, void* clientData, void*) {
        *static_cast<bool*>(clientData) = true;
    });
    reader->AddObserver(vtkCommand::ErrorEvent, observer);
    return observer;
}

template <typename Reader>
vtkSmartPointer<vtkDataObject> ReadXmlDataObject(const std::filesystem::path& path,
                                                  const std::string& fileName) {
    auto reader = vtkSmartPointer<Reader>::New();
    bool hadError = false;
    [[maybe_unused]] const auto errorObserver = ObserveVtkReaderErrors(reader, &hadError);
    [[maybe_unused]] const auto executiveErrorObserver =
        ObserveVtkReaderErrors(reader->GetExecutive(), &hadError);
    if (!reader->CanReadFile(fileName.c_str())) {
        throw std::runtime_error("Unsupported or unreadable VTK dataset: "
                                 + VisualizationPathToUtf8(path));
    }
    reader->SetFileName(fileName.c_str());
    reader->Update();
    vtkDataObject* output = reader->GetOutputDataObject(0);
    if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError || output == nullptr) {
        throw std::runtime_error("Unsupported or unreadable VTK dataset: "
                                 + VisualizationPathToUtf8(path));
    }
    return output;
}

inline vtkSmartPointer<vtkDataObject> ReadDataObject(const std::filesystem::path& path) {
    const std::string ext = ToLower(path.extension().string());
    const std::string fileName = VisualizationPathToUtf8(path);
    if (ext == ".vti") {
        return ReadXmlDataObject<vtkXMLImageDataReader>(path, fileName);
    }
    if (ext == ".vts") {
        return ReadXmlDataObject<vtkXMLStructuredGridReader>(path, fileName);
    }
    if (ext == ".vtr") {
        return ReadXmlDataObject<vtkXMLRectilinearGridReader>(path, fileName);
    }
    if (ext == ".vtu") {
        return ReadXmlDataObject<vtkXMLUnstructuredGridReader>(path, fileName);
    }
    if (ext == ".vtp") {
        return ReadXmlDataObject<vtkXMLPolyDataReader>(path, fileName);
    }
    if (ext == ".vtk") {
        auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
        bool hadError = false;
        [[maybe_unused]] const auto errorObserver = ObserveVtkReaderErrors(reader, &hadError);
        [[maybe_unused]] const auto executiveErrorObserver =
            ObserveVtkReaderErrors(reader->GetExecutive(), &hadError);
        reader->SetFileName(fileName.c_str());
        if (reader->ReadOutputType() < 0 || hadError) {
            throw std::runtime_error("Unsupported or unreadable VTK dataset: "
                                     + VisualizationPathToUtf8(path));
        }
        reader->Update();
        vtkDataObject* output = reader->GetOutputDataObject(0);
        if (hadError || reader->GetErrorCode() != vtkErrorCode::NoError || output == nullptr) {
            throw std::runtime_error("Unsupported or unreadable VTK dataset: "
                                     + VisualizationPathToUtf8(path));
        }
        return output;
    }
    return nullptr;
}

inline std::string DataObjectType(vtkDataObject* object) {
    if (object == nullptr) {
        return "unknown";
    }
    if (vtkImageData::SafeDownCast(object) != nullptr) {
        return "vtkImageData";
    }
    if (vtkStructuredGrid::SafeDownCast(object) != nullptr) {
        return "vtkStructuredGrid";
    }
    if (vtkRectilinearGrid::SafeDownCast(object) != nullptr) {
        return "vtkRectilinearGrid";
    }
    if (vtkUnstructuredGrid::SafeDownCast(object) != nullptr) {
        return "vtkUnstructuredGrid";
    }
    if (vtkPolyData::SafeDownCast(object) != nullptr) {
        return "vtkPolyData";
    }
    return object->GetClassName() != nullptr ? object->GetClassName() : "vtkDataObject";
}

inline void WriteDimensions(std::ostream& out, vtkDataObject* object, const std::string& indent) {
    int dims[3] = {0, 0, 0};
    bool hasDims = false;
    if (auto* image = vtkImageData::SafeDownCast(object)) {
        image->GetDimensions(dims);
        hasDims = true;
    } else if (auto* structured = vtkStructuredGrid::SafeDownCast(object)) {
        structured->GetDimensions(dims);
        hasDims = true;
    } else if (auto* rectilinear = vtkRectilinearGrid::SafeDownCast(object)) {
        rectilinear->GetDimensions(dims);
        hasDims = true;
    }
    if (!hasDims) {
        out << indent << "\"dimensions\": null";
        return;
    }
    out << indent << "\"dimensions\": [" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]";
}

inline void WriteArray(std::ostream& out, vtkAbstractArray* array, const std::string& indent) {
    const std::string name = array != nullptr && array->GetName() != nullptr ? array->GetName() : "";
    const int components = array != nullptr ? array->GetNumberOfComponents() : 0;
    const auto labels = ComponentLabels(name, components);
    std::string dataType = array != nullptr && array->GetDataTypeAsString() != nullptr ? array->GetDataTypeAsString() : "";
    out << indent << "{\n";
    out << indent << "  \"name\": " << Quote(name) << ",\n";
    out << indent << "  \"vtk_class\": " << Quote(array != nullptr && array->GetClassName() != nullptr ? array->GetClassName() : "") << ",\n";
    out << indent << "  \"data_type\": " << Quote(dataType) << ",\n";
    out << indent << "  \"number_of_components\": " << components << ",\n";
    out << indent << "  \"number_of_tuples\": " << (array != nullptr ? array->GetNumberOfTuples() : 0) << ",\n";
    out << indent << "  \"component_labels\": [";
    for (std::size_t i = 0; i < labels.size(); ++i) {
        out << (i == 0 ? "" : ", ") << Quote(labels[i]);
    }
    out << "],\n";
    out << indent << "  \"semantic_role\": " << Quote(SemanticRole(name, components)) << ",\n";
    out << indent << "  \"physical_meaning\": " << Quote(PhysicalMeaning(name, components)) << "\n";
    out << indent << "}";
}

inline void WriteArrayList(std::ostream& out,
                           vtkFieldData* arrays,
                           const std::string& key,
                           const std::string& indent) {
    out << indent << Quote(key) << ": [";
    if (arrays == nullptr || arrays->GetNumberOfArrays() == 0) {
        out << "]";
        return;
    }
    out << "\n";
    for (int i = 0; i < arrays->GetNumberOfArrays(); ++i) {
        WriteArray(out, arrays->GetAbstractArray(i), indent + "  ");
        out << (i + 1 == arrays->GetNumberOfArrays() ? "\n" : ",\n");
    }
    out << indent << "]";
}

inline void WritePvdFile(std::ostream& out,
                         const std::filesystem::path& path,
                         const std::filesystem::path& outputDir,
                         const std::string& indent) {
    out << indent << "{\n";
    out << indent << "  \"path\": " << Quote(RelativePath(path, outputDir)) << ",\n";
    out << indent << "  \"format\": \"pvd\",\n";
    out << indent << "  \"vtk_data_object\": \"vtkCollection\",\n";
    out << indent << "  \"physical_meaning\": \"PVD collection file referencing VTK-family datasets written by this solver.\",\n";
    out << indent << "  \"datasets\": [";
    const std::string fileName = VisualizationPathToUtf8(path);
    vtkSmartPointer<vtkXMLDataElement> root;
    root.TakeReference(vtkXMLUtilities::ReadElementFromFile(fileName.c_str()));
    if (root == nullptr || root->GetName() == nullptr
        || std::string(root->GetName()) != "VTKFile") {
        throw std::runtime_error("Cannot parse PVD collection: " + fileName);
    }
    const char* vtkFileType = root->GetAttribute("type");
    if (vtkFileType == nullptr || std::string(vtkFileType) != "Collection") {
        throw std::runtime_error("PVD VTKFile type must be Collection: " + fileName);
    }
    vtkXMLDataElement* collection = root->FindNestedElementWithName("Collection");
    if (collection == nullptr) {
        throw std::runtime_error("PVD collection does not contain a Collection element: "
                                 + fileName);
    }
    bool first = true;
    for (int elementIndex = 0;
         elementIndex < collection->GetNumberOfNestedElements();
         ++elementIndex) {
        vtkXMLDataElement* element = collection->GetNestedElement(elementIndex);
        if (element == nullptr || element->GetName() == nullptr
            || std::string(element->GetName()) != "DataSet") {
            continue;
        }
        const char* referencedFile = element->GetAttribute("file");
        if (referencedFile == nullptr || *referencedFile == '\0') {
            throw std::runtime_error("PVD DataSet element is missing a non-empty file attribute: "
                                     + fileName);
        }
        out << (first ? "\n" : ",\n");
        first = false;
        out << indent << "    {";
        for (int attributeIndex = 0;
             attributeIndex < element->GetNumberOfAttributes();
             ++attributeIndex) {
            const char* name = element->GetAttributeName(attributeIndex);
            const char* value = element->GetAttributeValue(attributeIndex);
            if (name == nullptr || value == nullptr) {
                throw std::runtime_error("Cannot read a PVD DataSet attribute: " + fileName);
            }
            out << (attributeIndex == 0 ? "" : ", ")
                << Quote(name) << ": " << Quote(value);
        }
        out << "}";
    }
    if (!first) {
        out << "\n" << indent << "  ";
    }
    out << "]\n";
    out << indent << "}";
}

inline void WriteVtkDataFile(std::ostream& out,
                             const std::filesystem::path& path,
                             const std::filesystem::path& outputDir,
                             const std::string& indent) {
    vtkSmartPointer<vtkDataObject> object = ReadDataObject(path);
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(object);
    out << indent << "{\n";
    out << indent << "  \"path\": " << Quote(RelativePath(path, outputDir)) << ",\n";
    out << indent << "  \"format\": " << Quote(ToLower(path.extension().string()).substr(1)) << ",\n";
    out << indent << "  \"vtk_data_object\": " << Quote(DataObjectType(object)) << ",\n";
    out << indent << "  \"point_count\": " << (dataSet != nullptr ? dataSet->GetNumberOfPoints() : 0) << ",\n";
    out << indent << "  \"cell_count\": " << (dataSet != nullptr ? dataSet->GetNumberOfCells() : 0) << ",\n";
    WriteDimensions(out, object, indent + "  ");
    out << ",\n";
    if (dataSet != nullptr) {
        WriteArrayList(out, dataSet->GetPointData(), "point_data", indent + "  ");
        out << ",\n";
        WriteArrayList(out, dataSet->GetCellData(), "cell_data", indent + "  ");
        out << ",\n";
    } else {
        out << indent << "  \"point_data\": [],\n";
        out << indent << "  \"cell_data\": [],\n";
    }
    WriteArrayList(out, object != nullptr ? object->GetFieldData() : nullptr, "field_data", indent + "  ");
    out << "\n";
    out << indent << "}";
}

inline void WriteVtkHdfDataFile(std::ostream& out,
                                const std::filesystem::path& path,
                                const std::filesystem::path& outputDir,
                                const std::string& indent) {
    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(path, ec);
    out << indent << "{\n";
    out << indent << "  \"path\": " << Quote(RelativePath(path, outputDir)) << ",\n";
    out << indent << "  \"format\": \"vtkhdf\",\n";
    out << indent << "  \"vtk_data_object\": \"vtkHDF\",\n";
    out << indent << "  \"point_count\": null,\n";
    out << indent << "  \"cell_count\": null,\n";
    out << indent << "  \"dimensions\": null,\n";
    out << indent << "  \"point_data\": [],\n";
    out << indent << "  \"cell_data\": [],\n";
    out << indent << "  \"field_data\": [],\n";
    out << indent << "  \"file_size_bytes\": " << (ec ? 0 : fileSize) << ",\n";
    out << indent << "  \"physical_meaning\": \"Chunked VTKHDF dataset written by this solver; use a VTKHDF reader or HDF5 inspection for full array metadata.\"\n";
    out << indent << "}";
}

inline bool IsVtkFamilyFile(const std::filesystem::path& path) {
    const std::string ext = ToLower(path.extension().string());
    return ext == ".vti" || ext == ".vts" || ext == ".vtr" || ext == ".vtu" ||
           ext == ".vtp" || ext == ".vtk" || ext == ".pvd" || ext == ".vtkhdf";
}

class TemporaryOutputManifestFile {
public:
    explicit TemporaryOutputManifestFile(std::filesystem::path path)
        : path_(std::move(path)) {}

    TemporaryOutputManifestFile(const TemporaryOutputManifestFile&) = delete;
    TemporaryOutputManifestFile& operator=(const TemporaryOutputManifestFile&) = delete;

    ~TemporaryOutputManifestFile() {
        if (active_) {
            std::error_code ignored;
            std::filesystem::remove(path_, ignored);
        }
    }

    void Release() noexcept {
        active_ = false;
    }

private:
    std::filesystem::path path_;
    bool active_ = true;
};

inline void ReplaceOutputManifestFile(const std::filesystem::path& temporaryPath,
                                      const std::filesystem::path& manifestPath) {
#ifdef _WIN32
    if (!MoveFileExW(temporaryPath.c_str(),
                     manifestPath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD error = GetLastError();
        std::error_code ignored;
        std::filesystem::remove(temporaryPath, ignored);
        throw std::runtime_error("Cannot atomically replace output manifest (Windows error "
                                 + std::to_string(error) + "): "
                                 + VisualizationPathToUtf8(manifestPath));
    }
#else
    std::error_code error;
    std::filesystem::rename(temporaryPath, manifestPath, error);
    if (error) {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Cannot atomically replace output manifest: "
                                 + VisualizationPathToUtf8(manifestPath) + ": "
                                 + error.message());
    }
#endif
}

inline void WriteOutputManifest(const std::filesystem::path& outputDir,
                                const std::string& solverName,
                                const std::filesystem::path& publishedOutputDir,
                                VisualizationCatalog catalog) {
    std::filesystem::create_directories(outputDir);
    std::vector<std::filesystem::path> files;
    if (std::filesystem::exists(outputDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(outputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::filesystem::path path = entry.path();
            if (IsVtkFamilyFile(path)) {
                const std::filesystem::path canonicalPath = std::filesystem::canonical(path);
                const std::filesystem::path canonicalOutput =
                    std::filesystem::weakly_canonical(std::filesystem::absolute(outputDir));
                if (!VisualizationPathIsWithin(canonicalPath, canonicalOutput)) {
                    throw std::runtime_error("Output manifest VTK file escapes the output directory: "
                                             + VisualizationPathToUtf8(path));
                }
                files.push_back(path);
            }
        }
    }
    std::sort(files.begin(), files.end());

    if (catalog.solverFamily.empty()) {
        catalog.solverFamily = CanonicalSolverFamily(solverName);
    }
    if (catalog.solverVariant.empty()) {
        catalog.solverVariant = SolverVariantFromName(solverName);
    }
    if (catalog.runId.empty()) {
        catalog.runId = MakeVisualizationRunId();
    }
    ValidateVisualizationCatalog(catalog, outputDir);

    const std::filesystem::path manifestPath = outputDir / "output_manifest.json";
    const std::filesystem::path temporaryPath =
        outputDir / ("output_manifest.json.tmp-" + MakeVisualizationRunId());
    TemporaryOutputManifestFile temporaryFile(temporaryPath);
    std::ofstream out(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot write temporary output manifest: "
                                 + VisualizationPathToUtf8(temporaryPath));
    }
    out.imbue(std::locale::classic());

    out << "{\n";
    out << "  \"schema\": \"streamcenterplus.output_manifest.v2\",\n";
    out << "  \"solver\": " << Quote(solverName) << ",\n";
    out << "  \"solver_family\": " << Quote(catalog.solverFamily) << ",\n";
    out << "  \"solver_variant\": " << Quote(catalog.solverVariant) << ",\n";
    out << "  \"run_id\": " << Quote(catalog.runId) << ",\n";
    out << "  \"output_directory\": "
        << Quote(VisualizationPathToUtf8(std::filesystem::absolute(publishedOutputDir)))
        << ",\n";
    out << "  \"naming_conventions\": {\n";
    out << "    \"velocity\": \"Velocity vectors are written as 3-component VTK arrays named with velocity, for example velocity, mean_velocity, phi_velocity, or velocity_rom.\",\n";
    out << "    \"pressure\": \"Pressure scalars are written with pressure in the name, for example pressure, mean_pressure, or pressure_rom.\",\n";
    out << "    \"components\": \"Only non-couplable component scalars use u/v/w component names.\"\n";
    out << "  },\n";
    out << "  \"vtk_files\": [";
    for (std::size_t i = 0; i < files.size(); ++i) {
        out << (i == 0 ? "\n" : ",\n");
        const std::string ext = ToLower(files[i].extension().string());
        if (ext == ".pvd") {
            WritePvdFile(out, files[i], outputDir, "    ");
        } else if (ext == ".vtkhdf") {
            WriteVtkHdfDataFile(out, files[i], outputDir, "    ");
        } else {
            WriteVtkDataFile(out, files[i], outputDir, "    ");
        }
    }
    if (!files.empty()) {
        out << "\n  ";
    }
    out << "],\n";
    out << "  \"visualization_catalog\": ";
    WriteVisualizationCatalog(out, catalog, "  ");
    out << "\n";
    out << "}\n";

    out.flush();
    if (!out) {
        out.close();
        throw std::runtime_error("Failed while writing output manifest: "
                                 + VisualizationPathToUtf8(temporaryPath));
    }
    out.close();
    if (!out) {
        throw std::runtime_error("Failed while closing output manifest: "
                                 + VisualizationPathToUtf8(temporaryPath));
    }
    ReplaceOutputManifestFile(temporaryPath, manifestPath);
    temporaryFile.Release();
}

inline void WriteOutputManifest(const std::filesystem::path& outputDir,
                                const std::string& solverName,
                                const std::filesystem::path& publishedOutputDir) {
    WriteOutputManifest(outputDir,
                        solverName,
                        publishedOutputDir,
                        MakeVisualizationCatalog(solverName));
}

inline void WriteOutputManifest(const std::filesystem::path& outputDir,
                                const std::string& solverName,
                                VisualizationCatalog catalog) {
    WriteOutputManifest(outputDir, solverName, outputDir, std::move(catalog));
}

inline void WriteOutputManifest(const std::filesystem::path& outputDir,
                                const std::string& solverName) {
    WriteOutputManifest(outputDir, solverName, outputDir);
}

}  // namespace streamcenterplus::manifest
