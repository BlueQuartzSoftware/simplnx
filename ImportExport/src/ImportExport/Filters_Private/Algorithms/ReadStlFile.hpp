#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadStlFileInputValues inputValues;

  inputValues.StlFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  inputValues.SurfaceMeshDataContainerName = filterArgs.value<DataPath>(k_SurfaceMeshDataContainerName_Key);
  inputValues.FaceAttributeMatrixName = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  inputValues.FaceNormalsArrayName = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  return ReadStlFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT ReadStlFileInputValues
{
  FileSystemPathParameter::ValueType StlFilePath;
  DataPath SurfaceMeshDataContainerName;
  DataPath FaceAttributeMatrixName;
  DataPath FaceNormalsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT ReadStlFile
{
public:
  ReadStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadStlFileInputValues* inputValues);
  ~ReadStlFile() noexcept;

  ReadStlFile(const ReadStlFile&) = delete;
  ReadStlFile(ReadStlFile&&) noexcept = delete;
  ReadStlFile& operator=(const ReadStlFile&) = delete;
  ReadStlFile& operator=(ReadStlFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
