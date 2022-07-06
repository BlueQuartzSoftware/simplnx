#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindVolFractionsInputValues inputValues;

  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.VolFractionsArrayPath = filterArgs.value<DataPath>(k_VolFractionsArrayPath_Key);

  return FindVolFractions(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindVolFractionsInputValues
{
  DataPath CellPhasesArrayPath;
  DataPath VolFractionsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindVolFractions
{
public:
  FindVolFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindVolFractionsInputValues* inputValues);
  ~FindVolFractions() noexcept;

  FindVolFractions(const FindVolFractions&) = delete;
  FindVolFractions(FindVolFractions&&) noexcept = delete;
  FindVolFractions& operator=(const FindVolFractions&) = delete;
  FindVolFractions& operator=(FindVolFractions&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindVolFractionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
