#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindNeighborsInputValues inputValues;

  inputValues.StoreBoundaryCells = filterArgs.value<bool>(k_StoreBoundaryCells_Key);
  inputValues.StoreSurfaceFeatures = filterArgs.value<bool>(k_StoreSurfaceFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.BoundaryCellsArrayName = filterArgs.value<DataPath>(k_BoundaryCellsArrayName_Key);
  inputValues.NumNeighborsArrayName = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  inputValues.NeighborListArrayName = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);
  inputValues.SharedSurfaceAreaListArrayName = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayName_Key);
  inputValues.SurfaceFeaturesArrayName = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayName_Key);

  return FindNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindNeighborsInputValues
{
  bool StoreBoundaryCells;
  bool StoreSurfaceFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath BoundaryCellsArrayName;
  DataPath NumNeighborsArrayName;
  DataPath NeighborListArrayName;
  DataPath SharedSurfaceAreaListArrayName;
  DataPath SurfaceFeaturesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindNeighbors
{
public:
  FindNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNeighborsInputValues* inputValues);
  ~FindNeighbors() noexcept;

  FindNeighbors(const FindNeighbors&) = delete;
  FindNeighbors(FindNeighbors&&) noexcept = delete;
  FindNeighbors& operator=(const FindNeighbors&) = delete;
  FindNeighbors& operator=(FindNeighbors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
