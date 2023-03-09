#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureReferenceMisorientationsInputValues inputValues;

  inputValues.ReferenceOrientation = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceOrientation_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.GBEuclideanDistancesArrayPath = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureReferenceMisorientationsArrayName = filterArgs.value<DataPath>(k_FeatureReferenceMisorientationsArrayName_Key);
  inputValues.FeatureAvgMisorientationsArrayName = filterArgs.value<DataPath>(k_FeatureAvgMisorientationsArrayName_Key);

  return FindFeatureReferenceMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceMisorientationsInputValues
{
  ChoicesParameter::ValueType ReferenceOrientation;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath GBEuclideanDistancesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureReferenceMisorientationsArrayName;
  DataPath FeatureAvgMisorientationsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceMisorientations
{
public:
  FindFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                      FindFeatureReferenceMisorientationsInputValues* inputValues);
  ~FindFeatureReferenceMisorientations() noexcept;

  FindFeatureReferenceMisorientations(const FindFeatureReferenceMisorientations&) = delete;
  FindFeatureReferenceMisorientations(FindFeatureReferenceMisorientations&&) noexcept = delete;
  FindFeatureReferenceMisorientations& operator=(const FindFeatureReferenceMisorientations&) = delete;
  FindFeatureReferenceMisorientations& operator=(FindFeatureReferenceMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureReferenceMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
