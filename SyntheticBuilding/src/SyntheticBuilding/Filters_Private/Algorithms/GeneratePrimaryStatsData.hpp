#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GeneratePrimaryStatsDataInputValues inputValues;

  inputValues.PhaseName = filterArgs.value<StringParameter::ValueType>(k_PhaseName_Key);
  inputValues.CrystalSymmetry = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalSymmetry_Key);
  inputValues.MicroPresetModel = filterArgs.value<ChoicesParameter::ValueType>(k_MicroPresetModel_Key);
  inputValues.PhaseFraction = filterArgs.value<float64>(k_PhaseFraction_Key);
  inputValues.Mu = filterArgs.value<float64>(k_Mu_Key);
  inputValues.Sigma = filterArgs.value<float64>(k_Sigma_Key);
  inputValues.MinCutOff = filterArgs.value<float64>(k_MinCutOff_Key);
  inputValues.MaxCutOff = filterArgs.value<float64>(k_MaxCutOff_Key);
  inputValues.BinStepSize = filterArgs.value<float64>(k_BinStepSize_Key);
  inputValues.NumberOfBins = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumberOfBins_Key);
  inputValues.FeatureESD = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FeatureESD_Key);
  inputValues.OdfData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OdfData_Key);
  inputValues.MdfData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MdfData_Key);
  inputValues.AxisOdfData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AxisOdfData_Key);
  inputValues.CreateEnsembleAttributeMatrix = filterArgs.value<bool>(k_CreateEnsembleAttributeMatrix_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.AppendToExistingAttributeMatrix = filterArgs.value<bool>(k_AppendToExistingAttributeMatrix_Key);
  inputValues.SelectedEnsembleAttributeMatrix = filterArgs.value<DataPath>(k_SelectedEnsembleAttributeMatrix_Key);

  return GeneratePrimaryStatsData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT GeneratePrimaryStatsDataInputValues
{
  StringParameter::ValueType PhaseName;
  ChoicesParameter::ValueType CrystalSymmetry;
  ChoicesParameter::ValueType MicroPresetModel;
  float64 PhaseFraction;
  float64 Mu;
  float64 Sigma;
  float64 MinCutOff;
  float64 MaxCutOff;
  float64 BinStepSize;
  <<<NOT_IMPLEMENTED>>> NumberOfBins;
  <<<NOT_IMPLEMENTED>>> FeatureESD;
  <<<NOT_IMPLEMENTED>>> OdfData;
  /*[x]*/<<<NOT_IMPLEMENTED>>> MdfData;
  /*[x]*/<<<NOT_IMPLEMENTED>>> AxisOdfData;
  /*[x]*/ bool CreateEnsembleAttributeMatrix;
  DataPath DataContainerName;
  DataPath CellEnsembleAttributeMatrixName;
  bool AppendToExistingAttributeMatrix;
  DataPath SelectedEnsembleAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT GeneratePrimaryStatsData
{
public:
  GeneratePrimaryStatsData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GeneratePrimaryStatsDataInputValues* inputValues);
  ~GeneratePrimaryStatsData() noexcept;

  GeneratePrimaryStatsData(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData(GeneratePrimaryStatsData&&) noexcept = delete;
  GeneratePrimaryStatsData& operator=(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData& operator=(GeneratePrimaryStatsData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GeneratePrimaryStatsDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
