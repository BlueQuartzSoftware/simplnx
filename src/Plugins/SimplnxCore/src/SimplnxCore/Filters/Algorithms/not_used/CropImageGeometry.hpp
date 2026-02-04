#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CropImageGeometryInputValues inputValues;
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(cell_feature_attribute_matrix_path);
  inputValues.CropXDim = filterArgs.value<BoolParameter::ValueType>(crop_x_dim);
  inputValues.CropYDim = filterArgs.value<BoolParameter::ValueType>(crop_y_dim);
  inputValues.CropZDim = filterArgs.value<BoolParameter::ValueType>(crop_z_dim);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.MaxCoord = filterArgs.value<VectorFloat64Parameter::ValueType>(max_coord);
  inputValues.MaxVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(max_voxel);
  inputValues.MinCoord = filterArgs.value<VectorFloat64Parameter::ValueType>(min_coord);
  inputValues.MinVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(min_voxel);
  inputValues.OutputImageGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(output_image_geometry_path);
  inputValues.RemoveOriginalGeometry = filterArgs.value<BoolParameter::ValueType>(remove_original_geometry);
  inputValues.RenumberFeatures = filterArgs.value<BoolParameter::ValueType>(renumber_features);
  inputValues.UsePhysicalBounds = filterArgs.value<BoolParameter::ValueType>(use_physical_bounds);
  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CropImageGeometryInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  BoolParameter::ValueType CropXDim;
  BoolParameter::ValueType CropYDim;
  BoolParameter::ValueType CropZDim;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  VectorFloat64Parameter::ValueType MaxCoord;
  VectorUInt64Parameter::ValueType MaxVoxel;
  VectorFloat64Parameter::ValueType MinCoord;
  VectorUInt64Parameter::ValueType MinVoxel;
  DataGroupCreationParameter::ValueType OutputImageGeometryPath;
  BoolParameter::ValueType RemoveOriginalGeometry;
  BoolParameter::ValueType RenumberFeatures;
  BoolParameter::ValueType UsePhysicalBounds;
};

/**
 * @class CropImageGeometry
 * @brief This algorithm implements support code for the CropImageGeometryFilter
 */

class SIMPLNXCORE_EXPORT CropImageGeometry
{
public:
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CropImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
