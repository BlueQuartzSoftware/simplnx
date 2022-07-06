#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MinSizeInputValues inputValues;

  inputValues.MinAllowedFeatureSize = filterArgs.value<int32>(k_MinAllowedFeatureSize_Key);
  inputValues.ApplyToSinglePhase = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  inputValues.PhaseNumber = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.NumCellsArrayPath = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return MinSize(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT MinSizeInputValues
{
  int32 MinAllowedFeatureSize;
  bool ApplyToSinglePhase;
  int32 PhaseNumber;
  DataPath FeatureIdsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath NumCellsArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT MinSize
{
public:
  MinSize(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MinSizeInputValues* inputValues);
  ~MinSize() noexcept;

  MinSize(const MinSize&) = delete;
  MinSize(MinSize&&) noexcept = delete;
  MinSize& operator=(const MinSize&) = delete;
  MinSize& operator=(MinSize&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MinSizeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
