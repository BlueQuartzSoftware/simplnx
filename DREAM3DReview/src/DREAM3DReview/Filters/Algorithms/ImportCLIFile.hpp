#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportCLIFileInputValues inputValues;

  inputValues.CLIFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_CLIFile_Key);
  inputValues.EdgeDataContainerName = filterArgs.value<StringParameter::ValueType>(k_EdgeDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  inputValues.LayerIdsArrayName = filterArgs.value<DataPath>(k_LayerIdsArrayName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  return ImportCLIFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportCLIFileInputValues
{
  FileSystemPathParameter::ValueType CLIFile;
  StringParameter::ValueType EdgeDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath EdgeAttributeMatrixName;
  DataPath LayerIdsArrayName;
  DataPath FeatureIdsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportCLIFile
{
public:
  ImportCLIFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportCLIFileInputValues* inputValues);
  ~ImportCLIFile() noexcept;

  ImportCLIFile(const ImportCLIFile&) = delete;
  ImportCLIFile(ImportCLIFile&&) noexcept = delete;
  ImportCLIFile& operator=(const ImportCLIFile&) = delete;
  ImportCLIFile& operator=(ImportCLIFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportCLIFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
