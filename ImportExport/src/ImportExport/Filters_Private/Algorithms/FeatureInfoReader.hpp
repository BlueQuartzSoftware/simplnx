#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FeatureInfoReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.CreateCellLevelArrays = filterArgs.value<bool>(k_CreateCellLevelArrays_Key);
  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayName = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  inputValues.CellEulerAnglesArrayName = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  inputValues.FeatureEulerAnglesArrayName = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  inputValues.Delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);

  return FeatureInfoReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT FeatureInfoReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool CreateCellLevelArrays;
  bool RenumberFeatures;
  DataPath CellAttributeMatrixName;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayName;
  DataPath CellEulerAnglesArrayName;
  DataPath CellFeatureAttributeMatrixName;
  DataPath FeaturePhasesArrayName;
  DataPath FeatureEulerAnglesArrayName;
  ChoicesParameter::ValueType Delimiter;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT FeatureInfoReader
{
public:
  FeatureInfoReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureInfoReaderInputValues* inputValues);
  ~FeatureInfoReader() noexcept;

  FeatureInfoReader(const FeatureInfoReader&) = delete;
  FeatureInfoReader(FeatureInfoReader&&) noexcept = delete;
  FeatureInfoReader& operator=(const FeatureInfoReader&) = delete;
  FeatureInfoReader& operator=(FeatureInfoReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FeatureInfoReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
