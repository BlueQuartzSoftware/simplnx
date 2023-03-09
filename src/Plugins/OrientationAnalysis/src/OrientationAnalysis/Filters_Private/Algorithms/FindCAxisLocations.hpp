#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindCAxisLocationsInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CAxisLocationsArrayName = filterArgs.value<DataPath>(k_CAxisLocationsArrayName_Key);

  return FindCAxisLocations(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindCAxisLocationsInputValues
{
  DataPath QuatsArrayPath;
  DataPath CAxisLocationsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindCAxisLocations
{
public:
  FindCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindCAxisLocationsInputValues* inputValues);
  ~FindCAxisLocations() noexcept;

  FindCAxisLocations(const FindCAxisLocations&) = delete;
  FindCAxisLocations(FindCAxisLocations&&) noexcept = delete;
  FindCAxisLocations& operator=(const FindCAxisLocations&) = delete;
  FindCAxisLocations& operator=(FindCAxisLocations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindCAxisLocationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
