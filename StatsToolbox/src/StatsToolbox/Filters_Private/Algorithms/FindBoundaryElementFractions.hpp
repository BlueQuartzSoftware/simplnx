#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindBoundaryElementFractionsInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.BoundaryCellsArrayPath = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  inputValues.BoundaryCellFractionsArrayPath = filterArgs.value<DataPath>(k_BoundaryCellFractionsArrayPath_Key);

  return FindBoundaryElementFractions(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindBoundaryElementFractionsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath BoundaryCellsArrayPath;
  DataPath BoundaryCellFractionsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindBoundaryElementFractions
{
public:
  FindBoundaryElementFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryElementFractionsInputValues* inputValues);
  ~FindBoundaryElementFractions() noexcept;

  FindBoundaryElementFractions(const FindBoundaryElementFractions&) = delete;
  FindBoundaryElementFractions(FindBoundaryElementFractions&&) noexcept = delete;
  FindBoundaryElementFractions& operator=(const FindBoundaryElementFractions&) = delete;
  FindBoundaryElementFractions& operator=(FindBoundaryElementFractions&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryElementFractionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
