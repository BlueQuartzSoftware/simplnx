#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportEbsdMontageInputValues inputValues;

  inputValues.InputFileListInfo = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  inputValues.MontageName = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.DefineScanOverlap = filterArgs.value<ChoicesParameter::ValueType>(k_DefineScanOverlap_Key);
  inputValues.ScanOverlapPixel = filterArgs.value<VectorInt32Parameter::ValueType>(k_ScanOverlapPixel_Key);
  inputValues.ScanOverlapPercent = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScanOverlapPercent_Key);
  inputValues.GenerateIPFColorMap = filterArgs.value<bool>(k_GenerateIPFColorMap_Key);
  inputValues.CellIPFColorsArrayName = filterArgs.value<StringParameter::ValueType>(k_CellIPFColorsArrayName_Key);

  return ImportEbsdMontage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ImportEbsdMontageInputValues
{
  <<<NOT_IMPLEMENTED>>> InputFileListInfo;
  /*[x]*/ StringParameter::ValueType MontageName;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType CellEnsembleAttributeMatrixName;
  ChoicesParameter::ValueType DefineScanOverlap;
  VectorInt32Parameter::ValueType ScanOverlapPixel;
  VectorFloat32Parameter::ValueType ScanOverlapPercent;
  bool GenerateIPFColorMap;
  StringParameter::ValueType CellIPFColorsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ImportEbsdMontage
{
public:
  ImportEbsdMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportEbsdMontageInputValues* inputValues);
  ~ImportEbsdMontage() noexcept;

  ImportEbsdMontage(const ImportEbsdMontage&) = delete;
  ImportEbsdMontage(ImportEbsdMontage&&) noexcept = delete;
  ImportEbsdMontage& operator=(const ImportEbsdMontage&) = delete;
  ImportEbsdMontage& operator=(ImportEbsdMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportEbsdMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
