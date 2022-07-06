#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CombineStlFilesInputValues inputValues;

  inputValues.StlFilesPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilesPath_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  inputValues.FaceAttributeMatrixName = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  inputValues.FaceNormalsArrayName = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  return CombineStlFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT CombineStlFilesInputValues
{
  FileSystemPathParameter::ValueType StlFilesPath;
  StringParameter::ValueType TriangleDataContainerName;
  DataPath FaceAttributeMatrixName;
  DataPath FaceNormalsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT CombineStlFiles
{
public:
  CombineStlFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineStlFilesInputValues* inputValues);
  ~CombineStlFiles() noexcept;

  CombineStlFiles(const CombineStlFiles&) = delete;
  CombineStlFiles(CombineStlFiles&&) noexcept = delete;
  CombineStlFiles& operator=(const CombineStlFiles&) = delete;
  CombineStlFiles& operator=(CombineStlFiles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineStlFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
