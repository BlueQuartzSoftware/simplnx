#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  MergeColoniesInputValues inputValues;

  inputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  inputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  inputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.IdentifyGlobAlpha = filterArgs.value<bool>(k_IdentifyGlobAlpha_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CellParentIdsArrayName = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  inputValues.GlobAlphaArrayName = filterArgs.value<DataPath>(k_GlobAlphaArrayName_Key);
  inputValues.NewCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.FeatureParentIdsArrayName = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  return MergeColonies(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT MergeColoniesInputValues
{
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNeighborListArrayPath;
  DataPath ContiguousNeighborListArrayPath;
  float32 AxisTolerance;
  float32 AngleTolerance;
  bool IdentifyGlobAlpha;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CellParentIdsArrayName;
  DataPath GlobAlphaArrayName;
  DataPath NewCellFeatureAttributeMatrixName;
  DataPath FeatureParentIdsArrayName;
  DataPath ActiveArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT MergeColonies
{
public:
  MergeColonies(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeColoniesInputValues* inputValues);
  ~MergeColonies() noexcept;

  MergeColonies(const MergeColonies&) = delete;
  MergeColonies(MergeColonies&&) noexcept = delete;
  MergeColonies& operator=(const MergeColonies&) = delete;
  MergeColonies& operator=(MergeColonies&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MergeColoniesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
