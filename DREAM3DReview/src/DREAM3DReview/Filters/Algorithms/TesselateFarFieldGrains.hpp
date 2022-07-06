#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TesselateFarFieldGrainsInputValues inputValues;

  inputValues.FeatureInputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_FeatureInputFileListInfo_Key);
  inputValues.OutputCellAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixName_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.CellPhasesArrayName = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  inputValues.OutputCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  inputValues.FeatureEulerAnglesArrayName = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  inputValues.ElasticStrainsArrayName = filterArgs.value<DataPath>(k_ElasticStrainsArrayName_Key);
  inputValues.OutputCellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  inputValues.CrystalStructuresArrayName = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);

  return TesselateFarFieldGrains(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT TesselateFarFieldGrainsInputValues
{
  GeneratedFileListParameter::ValueType FeatureInputFileListInfo;
  DataPath OutputCellAttributeMatrixName;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath CellPhasesArrayName;
  DataPath OutputCellFeatureAttributeMatrixName;
  DataPath FeaturePhasesArrayName;
  DataPath FeatureEulerAnglesArrayName;
  DataPath ElasticStrainsArrayName;
  DataPath OutputCellEnsembleAttributeMatrixName;
  DataPath CrystalStructuresArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT TesselateFarFieldGrains
{
public:
  TesselateFarFieldGrains(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TesselateFarFieldGrainsInputValues* inputValues);
  ~TesselateFarFieldGrains() noexcept;

  TesselateFarFieldGrains(const TesselateFarFieldGrains&) = delete;
  TesselateFarFieldGrains(TesselateFarFieldGrains&&) noexcept = delete;
  TesselateFarFieldGrains& operator=(const TesselateFarFieldGrains&) = delete;
  TesselateFarFieldGrains& operator=(TesselateFarFieldGrains&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TesselateFarFieldGrainsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
