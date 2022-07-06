#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureReferenceCAxisMisorientationsInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.AvgCAxesArrayPath = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  inputValues.FeatureAvgCAxisMisorientationsArrayName = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  inputValues.FeatureStdevCAxisMisorientationsArrayName = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  inputValues.FeatureReferenceCAxisMisorientationsArrayName = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  return FindFeatureReferenceCAxisMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceCAxisMisorientationsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath AvgCAxesArrayPath;
  DataPath FeatureAvgCAxisMisorientationsArrayName;
  DataPath FeatureStdevCAxisMisorientationsArrayName;
  DataPath FeatureReferenceCAxisMisorientationsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceCAxisMisorientations
{
public:
  FindFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                           FindFeatureReferenceCAxisMisorientationsInputValues* inputValues);
  ~FindFeatureReferenceCAxisMisorientations() noexcept;

  FindFeatureReferenceCAxisMisorientations(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;
  FindFeatureReferenceCAxisMisorientations& operator=(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations& operator=(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureReferenceCAxisMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
