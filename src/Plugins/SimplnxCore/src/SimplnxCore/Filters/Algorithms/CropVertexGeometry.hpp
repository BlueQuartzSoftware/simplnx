#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CropVertexGeometryInputValues inputValues;
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_vertex_geometry_path);
  inputValues.MaxPos = filterArgs.value<VectorFloat32Parameter::ValueType>(max_pos);
  inputValues.MinPos = filterArgs.value<VectorFloat32Parameter::ValueType>(min_pos);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(output_vertex_geometry_path);
  inputValues.TargetArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(target_array_paths);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(vertex_attribute_matrix_name);
  return CropVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CropVertexGeometryInputValues
{
  GeometrySelectionParameter::ValueType InputVertexGeometryPath;
  VectorFloat32Parameter::ValueType MaxPos;
  VectorFloat32Parameter::ValueType MinPos;
  DataGroupCreationParameter::ValueType OutputVertexGeometryPath;
  MultiArraySelectionParameter::ValueType TargetArrayPaths;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
};

/**
 * @class CropVertexGeometry
 * @brief This algorithm implements support code for the CropVertexGeometryFilter
 */

class SIMPLNXCORE_EXPORT CropVertexGeometry
{
public:
  CropVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropVertexGeometryInputValues* inputValues);
  ~CropVertexGeometry() noexcept;

  CropVertexGeometry(const CropVertexGeometry&) = delete;
  CropVertexGeometry(CropVertexGeometry&&) noexcept = delete;
  CropVertexGeometry& operator=(const CropVertexGeometry&) = delete;
  CropVertexGeometry& operator=(CropVertexGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CropVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
