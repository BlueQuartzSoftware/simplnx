#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateEnsembleInfoInputValues inputValues;

  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.Ensemble = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Ensemble_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.CrystalStructuresArrayName = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  inputValues.PhaseTypesArrayName = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  inputValues.PhaseNamesArrayName = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  return CreateEnsembleInfo(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfoInputValues
{
  DataPath DataContainerName;
  <<<NOT_IMPLEMENTED>>> Ensemble;
  /*[x]*/ DataPath CellEnsembleAttributeMatrixName;
  DataPath CrystalStructuresArrayName;
  DataPath PhaseTypesArrayName;
  DataPath PhaseNamesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfo
{
public:
  CreateEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateEnsembleInfoInputValues* inputValues);
  ~CreateEnsembleInfo() noexcept;

  CreateEnsembleInfo(const CreateEnsembleInfo&) = delete;
  CreateEnsembleInfo(CreateEnsembleInfo&&) noexcept = delete;
  CreateEnsembleInfo& operator=(const CreateEnsembleInfo&) = delete;
  CreateEnsembleInfo& operator=(CreateEnsembleInfo&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateEnsembleInfoInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
