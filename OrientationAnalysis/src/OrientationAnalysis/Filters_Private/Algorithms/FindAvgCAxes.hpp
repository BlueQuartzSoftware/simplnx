#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindAvgCAxesInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.AvgCAxesArrayPath = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);

  return FindAvgCAxes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindAvgCAxesInputValues
{
  DataPath QuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath AvgCAxesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindAvgCAxes
{
public:
  FindAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgCAxesInputValues* inputValues);
  ~FindAvgCAxes() noexcept;

  FindAvgCAxes(const FindAvgCAxes&) = delete;
  FindAvgCAxes(FindAvgCAxes&&) noexcept = delete;
  FindAvgCAxes& operator=(const FindAvgCAxes&) = delete;
  FindAvgCAxes& operator=(FindAvgCAxes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindAvgCAxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
