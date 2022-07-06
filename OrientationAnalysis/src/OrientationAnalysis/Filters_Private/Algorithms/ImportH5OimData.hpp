#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportH5OimDataInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.SelectedScanNames = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedScanNames_Key);
  inputValues.ZSpacing = filterArgs.value<float64>(k_ZSpacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.ReadPatternData = filterArgs.value<bool>(k_ReadPatternData_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  return ImportH5OimData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ImportH5OimDataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  <<<NOT_IMPLEMENTED>>> SelectedScanNames;
  /*[x]*/ float64 ZSpacing;
  VectorFloat32Parameter::ValueType Origin;
  bool ReadPatternData;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath CellEnsembleAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ImportH5OimData
{
public:
  ImportH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportH5OimDataInputValues* inputValues);
  ~ImportH5OimData() noexcept;

  ImportH5OimData(const ImportH5OimData&) = delete;
  ImportH5OimData(ImportH5OimData&&) noexcept = delete;
  ImportH5OimData& operator=(const ImportH5OimData&) = delete;
  ImportH5OimData& operator=(ImportH5OimData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportH5OimDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
