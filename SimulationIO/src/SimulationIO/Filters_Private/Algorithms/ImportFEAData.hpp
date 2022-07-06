#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportFEADataInputValues inputValues;

  inputValues.FEAPackage = filterArgs.value<ChoicesParameter::ValueType>(k_FEAPackage_Key);
  inputValues.odbName = filterArgs.value<StringParameter::ValueType>(k_odbName_Key);
  inputValues.odbFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_odbFilePath_Key);
  inputValues.ABQPythonCommand = filterArgs.value<StringParameter::ValueType>(k_ABQPythonCommand_Key);
  inputValues.InstanceName = filterArgs.value<StringParameter::ValueType>(k_InstanceName_Key);
  inputValues.Step = filterArgs.value<StringParameter::ValueType>(k_Step_Key);
  inputValues.FrameNumber = filterArgs.value<int32>(k_FrameNumber_Key);
  inputValues.BSAMInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_BSAMInputFile_Key);
  inputValues.DEFORMInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMInputFile_Key);
  inputValues.DEFORMPointTrackInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMPointTrackInputFile_Key);
  inputValues.ImportSingleTimeStep = filterArgs.value<bool>(k_ImportSingleTimeStep_Key);
  inputValues.SingleTimeStepValue = filterArgs.value<int32>(k_SingleTimeStepValue_Key);
  inputValues.TimeSeriesBundleName = filterArgs.value<StringParameter::ValueType>(k_TimeSeriesBundleName_Key);
  inputValues.DataContainerName = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  return ImportFEAData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ImportFEADataInputValues
{
  ChoicesParameter::ValueType FEAPackage;
  StringParameter::ValueType odbName;
  FileSystemPathParameter::ValueType odbFilePath;
  StringParameter::ValueType ABQPythonCommand;
  StringParameter::ValueType InstanceName;
  StringParameter::ValueType Step;
  int32 FrameNumber;
  FileSystemPathParameter::ValueType BSAMInputFile;
  FileSystemPathParameter::ValueType DEFORMInputFile;
  FileSystemPathParameter::ValueType DEFORMPointTrackInputFile;
  bool ImportSingleTimeStep;
  int32 SingleTimeStepValue;
  StringParameter::ValueType TimeSeriesBundleName;
  StringParameter::ValueType DataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ImportFEAData
{
public:
  ImportFEAData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportFEADataInputValues* inputValues);
  ~ImportFEAData() noexcept;

  ImportFEAData(const ImportFEAData&) = delete;
  ImportFEAData(ImportFEAData&&) noexcept = delete;
  ImportFEAData& operator=(const ImportFEAData&) = delete;
  ImportFEAData& operator=(ImportFEAData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportFEADataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
