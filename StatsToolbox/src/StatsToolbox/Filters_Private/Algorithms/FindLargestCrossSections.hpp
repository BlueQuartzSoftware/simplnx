#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindLargestCrossSectionsInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.LargestCrossSectionsArrayPath = filterArgs.value<DataPath>(k_LargestCrossSectionsArrayPath_Key);

  return FindLargestCrossSections(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindLargestCrossSectionsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath FeatureIdsArrayPath;
  DataPath LargestCrossSectionsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindLargestCrossSections
{
public:
  FindLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindLargestCrossSectionsInputValues* inputValues);
  ~FindLargestCrossSections() noexcept;

  FindLargestCrossSections(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections(FindLargestCrossSections&&) noexcept = delete;
  FindLargestCrossSections& operator=(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections& operator=(FindLargestCrossSections&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
