#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateBsamFileInputValues inputValues;

  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.OutputFilePrefix = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  inputValues.NumClusters = filterArgs.value<int32>(k_NumClusters_Key);

  return CreateBsamFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT CreateBsamFileInputValues
{
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType OutputFilePrefix;
  int32 NumClusters;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT CreateBsamFile
{
public:
  CreateBsamFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateBsamFileInputValues* inputValues);
  ~CreateBsamFile() noexcept;

  CreateBsamFile(const CreateBsamFile&) = delete;
  CreateBsamFile(CreateBsamFile&&) noexcept = delete;
  CreateBsamFile& operator=(const CreateBsamFile&) = delete;
  CreateBsamFile& operator=(CreateBsamFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateBsamFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
