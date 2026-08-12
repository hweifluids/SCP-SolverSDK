#include <streamcenterplus/VtkInputAssociation.h>

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int Fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

vtkSmartPointer<vtkImageData> MakeImage(vtkIdType tupleCount) {
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(3, 3, 1);

    auto values = vtkSmartPointer<vtkDoubleArray>::New();
    values->SetName("cell_values");
    values->SetNumberOfComponents(1);
    values->SetNumberOfTuples(tupleCount);
    values->FillComponent(0, 1.0);
    image->GetCellData()->AddArray(values);
    return image;
}

bool ThrowsContaining(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

}  // namespace

int main() {
    vtkSmartPointer<vtkImageData> valid = MakeImage(4);
    vtkSmartPointer<vtkDataSet> centered =
        streamcenterplus::vtkinput::CellDataAtCellCenters(valid);
    vtkAbstractArray* centeredValues = centered->GetPointData()->GetAbstractArray("cell_values");
    if (centered->GetNumberOfPoints() != 4 || centeredValues == nullptr
        || centeredValues->GetNumberOfTuples() != 4) {
        return Fail("Valid CellData was not converted to a consistent point representation.");
    }

    vtkSmartPointer<vtkImageData> undersized = MakeImage(1);
    if (!ThrowsContaining(
            [&] { streamcenterplus::vtkinput::CellDataAtCellCenters(undersized); },
            "has 1 tuples; expected 4")) {
        return Fail("An undersized CellData array was accepted.");
    }

    vtkSmartPointer<vtkImageData> oversized = MakeImage(5);
    if (!ThrowsContaining(
            [&] { streamcenterplus::vtkinput::CellDataAtCellCenters(oversized); },
            "has 5 tuples; expected 4")) {
        return Fail("An oversized CellData array was accepted.");
    }

    return 0;
}
