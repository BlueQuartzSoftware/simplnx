#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateAbaqusFileInputValues inputValues;

  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.OutputFilePrefix = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  inputValues.JobName = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.NumDepvar = filterArgs.value<int32>(k_NumDepvar_Key);
  inputValues.NumUserOutVar = filterArgs.value<int32>(k_NumUserOutVar_Key);
  inputValues.MatConst = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MatConst_Key);
  inputValues.AbqFeatureIdsArrayPath = filterArgs.value<DataPath>(k_AbqFeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return CreateAbaqusFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT CreateAbaqusFileInputValues
{
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType OutputFilePrefix;
  StringParameter::ValueType JobName;
  int32 NumDepvar;
  int32 NumUserOutVar;
  <<<NOT_IMPLEMENTED>>> MatConst;
  /*[x]*/ DataPath AbqFeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT CreateAbaqusFile
{
public:
  CreateAbaqusFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAbaqusFileInputValues* inputValues);
  ~CreateAbaqusFile() noexcept;

  CreateAbaqusFile(const CreateAbaqusFile&) = delete;
  CreateAbaqusFile(CreateAbaqusFile&&) noexcept = delete;
  CreateAbaqusFile& operator=(const CreateAbaqusFile&) = delete;
  CreateAbaqusFile& operator=(CreateAbaqusFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateAbaqusFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
