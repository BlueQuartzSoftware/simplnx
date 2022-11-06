#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindAvgScalarValueForFeaturesInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.NewFeatureArrayArrayPath = filterArgs.value<DataPath>(k_NewFeatureArrayArrayPath_Key);

  return FindAvgScalarValueForFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindAvgScalarValueForFeaturesInputValues
{
  DataPath SelectedCellArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath NewFeatureArrayArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindAvgScalarValueForFeatures
{
public:
  FindAvgScalarValueForFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgScalarValueForFeaturesInputValues* inputValues);
  ~FindAvgScalarValueForFeatures() noexcept;

  FindAvgScalarValueForFeatures(const FindAvgScalarValueForFeatures&) = delete;
  FindAvgScalarValueForFeatures(FindAvgScalarValueForFeatures&&) noexcept = delete;
  FindAvgScalarValueForFeatures& operator=(const FindAvgScalarValueForFeatures&) = delete;
  FindAvgScalarValueForFeatures& operator=(FindAvgScalarValueForFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindAvgScalarValueForFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
