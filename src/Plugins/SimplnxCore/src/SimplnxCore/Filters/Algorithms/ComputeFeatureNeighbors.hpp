#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeFeatureNeighborsInputValues inputValues;
  inputValues.BoundaryCellsName = filterArgs.value<DataObjectNameParameter::ValueType>(boundary_cells_name);
  inputValues.CellFeatureArrayPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(cell_feature_array_path);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.NeighborListName = filterArgs.value<DataObjectNameParameter::ValueType>(neighbor_list_name);
  inputValues.NumberOfNeighborsName = filterArgs.value<DataObjectNameParameter::ValueType>(number_of_neighbors_name);
  inputValues.SharedSurfaceAreaListName = filterArgs.value<DataObjectNameParameter::ValueType>(shared_surface_area_list_name);
  inputValues.StoreBoundaryCells = filterArgs.value<BoolParameter::ValueType>(store_boundary_cells);
  inputValues.StoreSurfaceFeatures = filterArgs.value<BoolParameter::ValueType>(store_surface_features);
  inputValues.SurfaceFeaturesName = filterArgs.value<DataObjectNameParameter::ValueType>(surface_features_name);
  return ComputeFeatureNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureNeighborsInputValues
{
  DataObjectNameParameter::ValueType BoundaryCellsName;
  AttributeMatrixSelectionParameter::ValueType CellFeatureArrayPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataObjectNameParameter::ValueType NeighborListName;
  DataObjectNameParameter::ValueType NumberOfNeighborsName;
  DataObjectNameParameter::ValueType SharedSurfaceAreaListName;
  BoolParameter::ValueType StoreBoundaryCells;
  BoolParameter::ValueType StoreSurfaceFeatures;
  DataObjectNameParameter::ValueType SurfaceFeaturesName;
};

/**
 * @class ComputeFeatureNeighbors
 * @brief This algorithm implements support code for the ComputeFeatureNeighborsFilter
 */

class SIMPLNXCORE_EXPORT ComputeFeatureNeighbors
{
public:
  ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighbors() noexcept;

  ComputeFeatureNeighbors(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors(ComputeFeatureNeighbors&&) noexcept = delete;
  ComputeFeatureNeighbors& operator=(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors& operator=(ComputeFeatureNeighbors&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
