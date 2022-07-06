#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MinNeighborsInputValues inputValues;

  inputValues.MinNumNeighbors = filterArgs.value<int32>(k_MinNumNeighbors_Key);
  inputValues.ApplyToSinglePhase = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  inputValues.PhaseNumber = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.NumNeighborsArrayPath = filterArgs.value<DataPath>(k_NumNeighborsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return MinNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT MinNeighborsInputValues
{
  int32 MinNumNeighbors;
  bool ApplyToSinglePhase;
  int32 PhaseNumber;
  DataPath FeatureIdsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath NumNeighborsArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT MinNeighbors
{
public:
  MinNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MinNeighborsInputValues* inputValues);
  ~MinNeighbors() noexcept;

  MinNeighbors(const MinNeighbors&) = delete;
  MinNeighbors(MinNeighbors&&) noexcept = delete;
  MinNeighbors& operator=(const MinNeighbors&) = delete;
  MinNeighbors& operator=(MinNeighbors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MinNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
