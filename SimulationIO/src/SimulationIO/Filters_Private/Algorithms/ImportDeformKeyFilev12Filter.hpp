#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportDeformKeyFilev12FilterInputValues inputValues;

  inputValues.DEFORMInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMInputFile_Key);
  inputValues.VerboseOutput = filterArgs.value<bool>(k_VerboseOutput_Key);
  inputValues.DataContainerName = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  return ImportDeformKeyFilev12Filter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ImportDeformKeyFilev12FilterInputValues
{
  FileSystemPathParameter::ValueType DEFORMInputFile;
  bool VerboseOutput;
  StringParameter::ValueType DataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ImportDeformKeyFilev12Filter
{
public:
  ImportDeformKeyFilev12Filter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportDeformKeyFilev12FilterInputValues* inputValues);
  ~ImportDeformKeyFilev12Filter() noexcept;

  ImportDeformKeyFilev12Filter(const ImportDeformKeyFilev12Filter&) = delete;
  ImportDeformKeyFilev12Filter(ImportDeformKeyFilev12Filter&&) noexcept = delete;
  ImportDeformKeyFilev12Filter& operator=(const ImportDeformKeyFilev12Filter&) = delete;
  ImportDeformKeyFilev12Filter& operator=(ImportDeformKeyFilev12Filter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportDeformKeyFilev12FilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
