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

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureNeighborsInputValues
{
  DataPath BoundaryCellsPath;
  AttributeMatrixSelectionParameter::ValueType CellFeatureArrayPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataPath NeighborListPath;
  DataPath NumberOfNeighborsPath;
  DataPath SharedSurfaceAreaListPath;
  BoolParameter::ValueType StoreBoundaryCells;
  BoolParameter::ValueType StoreSurfaceFeatures;
  DataPath SurfaceFeaturesPath;
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
