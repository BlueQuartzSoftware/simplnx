#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportDelamDataInputValues inputValues;

  inputValues.CSDGMFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_CSDGMFile_Key);
  inputValues.BvidStdOutFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_BvidStdOutFile_Key);
  inputValues.InterfaceThickness = filterArgs.value<float32>(k_InterfaceThickness_Key);
  inputValues.DataContainerPath = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.DataArrayName = filterArgs.value<StringParameter::ValueType>(k_DataArrayName_Key);

  return ImportDelamData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ImportDelamDataInputValues
{
  FileSystemPathParameter::ValueType CSDGMFile;
  FileSystemPathParameter::ValueType BvidStdOutFile;
  float32 InterfaceThickness;
  DataPath DataContainerPath;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType DataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ImportDelamData
{
public:
  ImportDelamData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportDelamDataInputValues* inputValues);
  ~ImportDelamData() noexcept;

  ImportDelamData(const ImportDelamData&) = delete;
  ImportDelamData(ImportDelamData&&) noexcept = delete;
  ImportDelamData& operator=(const ImportDelamData&) = delete;
  ImportDelamData& operator=(ImportDelamData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportDelamDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
