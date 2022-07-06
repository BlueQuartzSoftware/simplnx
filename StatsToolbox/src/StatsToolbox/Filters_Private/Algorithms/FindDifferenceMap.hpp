#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindDifferenceMapInputValues inputValues;

  inputValues.FirstInputArrayPath = filterArgs.value<DataPath>(k_FirstInputArrayPath_Key);
  inputValues.SecondInputArrayPath = filterArgs.value<DataPath>(k_SecondInputArrayPath_Key);
  inputValues.DifferenceMapArrayPath = filterArgs.value<DataPath>(k_DifferenceMapArrayPath_Key);

  return FindDifferenceMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindDifferenceMapInputValues
{
  DataPath FirstInputArrayPath;
  DataPath SecondInputArrayPath;
  DataPath DifferenceMapArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindDifferenceMap
{
public:
  FindDifferenceMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindDifferenceMapInputValues* inputValues);
  ~FindDifferenceMap() noexcept;

  FindDifferenceMap(const FindDifferenceMap&) = delete;
  FindDifferenceMap(FindDifferenceMap&&) noexcept = delete;
  FindDifferenceMap& operator=(const FindDifferenceMap&) = delete;
  FindDifferenceMap& operator=(FindDifferenceMap&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindDifferenceMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
