#pragma once

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace streamcenterplus::vtkinput {

struct PreparedDataSet {
    vtkSmartPointer<vtkDataSet> dataSet;
    bool sourceWasCellData = false;
};

inline std::array<int, 3> StructuredPointDimensions(vtkDataSet* dataSet) {
    if (dataSet == nullptr) {
        throw std::runtime_error("Cannot inspect dimensions of a null VTK dataset.");
    }

    std::array<int, 3> dimensions = {0, 0, 0};
    if (auto* image = vtkImageData::SafeDownCast(dataSet)) {
        image->GetDimensions(dimensions.data());
    } else if (auto* rectilinear = vtkRectilinearGrid::SafeDownCast(dataSet)) {
        rectilinear->GetDimensions(dimensions.data());
    } else if (auto* structured = vtkStructuredGrid::SafeDownCast(dataSet)) {
        structured->GetDimensions(dimensions.data());
    } else {
        throw std::runtime_error(
            "CellData compatibility requires vtkImageData, vtkRectilinearGrid, or vtkStructuredGrid input.");
    }
    return dimensions;
}

inline vtkSmartPointer<vtkDataSet> CellDataAtCellCenters(vtkDataSet* source) {
    if (source == nullptr) {
        throw std::runtime_error("Cannot convert CellData from a null VTK dataset.");
    }
    vtkCellData* cellData = source->GetCellData();
    if (cellData == nullptr || cellData->GetNumberOfArrays() == 0) {
        throw std::runtime_error("VTK CellData does not contain any arrays.");
    }

    const vtkIdType cellCount = source->GetNumberOfCells();
    if (cellCount <= 0) {
        throw std::runtime_error("VTK CellData input does not contain any cells.");
    }

    const std::array<int, 3> pointDimensions = StructuredPointDimensions(source);
    std::array<int, 3> cellDimensions = {
        pointDimensions[0] > 1 ? pointDimensions[0] - 1 : 1,
        pointDimensions[1] > 1 ? pointDimensions[1] - 1 : 1,
        pointDimensions[2] > 1 ? pointDimensions[2] - 1 : 1,
    };
    const vtkIdType structuredCellCount =
        static_cast<vtkIdType>(cellDimensions[0]) *
        static_cast<vtkIdType>(cellDimensions[1]) *
        static_cast<vtkIdType>(cellDimensions[2]);
    if (structuredCellCount != cellCount) {
        throw std::runtime_error(
            "CellData tuple layout does not match a structured cell-center grid.");
    }

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetDataTypeToDouble();
    points->SetNumberOfPoints(cellCount);
    for (vtkIdType cellId = 0; cellId < cellCount; ++cellId) {
        vtkCell* cell = source->GetCell(cellId);
        if (cell == nullptr || cell->GetNumberOfPoints() <= 0) {
            throw std::runtime_error("Failed to access a VTK cell while constructing cell-center coordinates.");
        }
        double parametricCenter[3] = {0.0, 0.0, 0.0};
        cell->GetParametricCenter(parametricCenter);
        int subId = 0;
        double center[3] = {0.0, 0.0, 0.0};
        std::vector<double> weights(static_cast<std::size_t>(cell->GetNumberOfPoints()), 0.0);
        cell->EvaluateLocation(subId, parametricCenter, center, weights.data());
        points->SetPoint(cellId, center);
    }

    auto centered = vtkSmartPointer<vtkStructuredGrid>::New();
    centered->SetDimensions(cellDimensions[0], cellDimensions[1], cellDimensions[2]);
    centered->SetPoints(points);
    centered->GetPointData()->DeepCopy(cellData);
    if (source->GetFieldData() != nullptr) {
        centered->GetFieldData()->DeepCopy(source->GetFieldData());
    }
    return centered;
}

inline vtkSmartPointer<vtkDataSet> UseSourceAssociation(
    const vtkSmartPointer<vtkDataSet>& source,
    bool sourceWasCellData) {
    if (source == nullptr) {
        throw std::runtime_error("Cannot prepare a null VTK dataset.");
    }
    return sourceWasCellData ? CellDataAtCellCenters(source) : source;
}

template <typename Validator>
PreparedDataSet PreparePointOrCellData(const vtkSmartPointer<vtkDataSet>& source,
                                       Validator&& validatePointRepresentation,
                                       const std::string& inputDescription) {
    if (source == nullptr) {
        throw std::runtime_error("Cannot prepare a null VTK dataset for " + inputDescription + ".");
    }

    std::string pointError;
    try {
        validatePointRepresentation(source);
        return {source, false};
    } catch (const std::exception& ex) {
        pointError = ex.what();
    }

    vtkSmartPointer<vtkDataSet> centered;
    try {
        centered = CellDataAtCellCenters(source);
        validatePointRepresentation(centered);
        return {centered, true};
    } catch (const std::exception& ex) {
        throw std::runtime_error(
            inputDescription + " was not found in PointData or CellData. PointData: " +
            pointError + "; CellData: " + ex.what());
    }
}

}  // namespace streamcenterplus::vtkinput
