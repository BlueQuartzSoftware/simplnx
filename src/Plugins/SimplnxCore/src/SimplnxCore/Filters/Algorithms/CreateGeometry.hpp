#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateGeometryInputValues inputValues;
  inputValues.ArrayHandlingIndex = filterArgs.value<ChoicesParameter::ValueType>(array_handling_index);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(cell_attribute_matrix_name);
  inputValues.Dimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(dimensions);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(edge_attribute_matrix_name);
  inputValues.EdgeListPath = filterArgs.value<ArraySelectionParameter::ValueType>(edge_list_path);
  inputValues.FaceAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(face_attribute_matrix_name);
  inputValues.GeometryTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(geometry_type_index);
  inputValues.HexahedralListPath = filterArgs.value<ArraySelectionParameter::ValueType>(hexahedral_list_path);
  inputValues.LengthUnitIndex = filterArgs.value<ChoicesParameter::ValueType>(length_unit_index);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(origin);
  inputValues.OutputGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(output_geometry_path);
  inputValues.QuadrilateralListPath = filterArgs.value<ArraySelectionParameter::ValueType>(quadrilateral_list_path);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(spacing);
  inputValues.TetrahedralListPath = filterArgs.value<ArraySelectionParameter::ValueType>(tetrahedral_list_path);
  inputValues.TriangleListPath = filterArgs.value<ArraySelectionParameter::ValueType>(triangle_list_path);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(vertex_attribute_matrix_name);
  inputValues.VertexListPath = filterArgs.value<ArraySelectionParameter::ValueType>(vertex_list_path);
  inputValues.WarningsAsErrors = filterArgs.value<BoolParameter::ValueType>(warnings_as_errors);
  inputValues.XBoundsPath = filterArgs.value<ArraySelectionParameter::ValueType>(x_bounds_path);
  inputValues.YBoundsPath = filterArgs.value<ArraySelectionParameter::ValueType>(y_bounds_path);
  inputValues.ZBoundsPath = filterArgs.value<ArraySelectionParameter::ValueType>(z_bounds_path);
  return CreateGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateGeometryInputValues
{
  ChoicesParameter::ValueType ArrayHandlingIndex;
  DataObjectNameParameter::ValueType CellAttributeMatrixName;
  VectorUInt64Parameter::ValueType Dimensions;
  DataObjectNameParameter::ValueType EdgeAttributeMatrixName;
  ArraySelectionParameter::ValueType EdgeListPath;
  DataObjectNameParameter::ValueType FaceAttributeMatrixName;
  ChoicesParameter::ValueType GeometryTypeIndex;
  ArraySelectionParameter::ValueType HexahedralListPath;
  ChoicesParameter::ValueType LengthUnitIndex;
  VectorFloat32Parameter::ValueType Origin;
  DataGroupCreationParameter::ValueType OutputGeometryPath;
  ArraySelectionParameter::ValueType QuadrilateralListPath;
  VectorFloat32Parameter::ValueType Spacing;
  ArraySelectionParameter::ValueType TetrahedralListPath;
  ArraySelectionParameter::ValueType TriangleListPath;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
  ArraySelectionParameter::ValueType VertexListPath;
  BoolParameter::ValueType WarningsAsErrors;
  ArraySelectionParameter::ValueType XBoundsPath;
  ArraySelectionParameter::ValueType YBoundsPath;
  ArraySelectionParameter::ValueType ZBoundsPath;
};

/**
 * @class CreateGeometry
 * @brief This algorithm implements support code for the CreateGeometryFilter
 */

class SIMPLNXCORE_EXPORT CreateGeometry
{
public:
  CreateGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateGeometryInputValues* inputValues);
  ~CreateGeometry() noexcept;

  CreateGeometry(const CreateGeometry&) = delete;
  CreateGeometry(CreateGeometry&&) noexcept = delete;
  CreateGeometry& operator=(const CreateGeometry&) = delete;
  CreateGeometry& operator=(CreateGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
