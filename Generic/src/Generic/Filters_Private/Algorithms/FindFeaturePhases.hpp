#pragma once

#include "Generic/Generic_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeaturePhasesInputValues inputValues;

  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);

  return FindFeaturePhases(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct GENERIC_EXPORT FindFeaturePhasesInputValues
{
  DataPath CellPhasesArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath FeaturePhasesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class GENERIC_EXPORT FindFeaturePhases
{
public:
  FindFeaturePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeaturePhasesInputValues* inputValues);
  ~FindFeaturePhases() noexcept;

  FindFeaturePhases(const FindFeaturePhases&) = delete;
  FindFeaturePhases(FindFeaturePhases&&) noexcept = delete;
  FindFeaturePhases& operator=(const FindFeaturePhases&) = delete;
  FindFeaturePhases& operator=(FindFeaturePhases&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeaturePhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
