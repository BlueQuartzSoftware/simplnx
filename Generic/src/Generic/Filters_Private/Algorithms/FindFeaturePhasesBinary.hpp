#pragma once

#include "Generic/Generic_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeaturePhasesBinaryInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  return FindFeaturePhasesBinary(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct GENERIC_EXPORT FindFeaturePhasesBinaryInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CellEnsembleAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class GENERIC_EXPORT FindFeaturePhasesBinary
{
public:
  FindFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeaturePhasesBinaryInputValues* inputValues);
  ~FindFeaturePhasesBinary() noexcept;

  FindFeaturePhasesBinary(const FindFeaturePhasesBinary&) = delete;
  FindFeaturePhasesBinary(FindFeaturePhasesBinary&&) noexcept = delete;
  FindFeaturePhasesBinary& operator=(const FindFeaturePhasesBinary&) = delete;
  FindFeaturePhasesBinary& operator=(FindFeaturePhasesBinary&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeaturePhasesBinaryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
