#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WriteTriangleGeometryInputValues inputValues;

  inputValues.OutputNodesFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputNodesFile_Key);
  inputValues.OutputTrianglesFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputTrianglesFile_Key);
  inputValues.DataContainerSelection = filterArgs.value<DataPath>(k_DataContainerSelection_Key);

  return WriteTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT WriteTriangleGeometryInputValues
{
  FileSystemPathParameter::ValueType OutputNodesFile;
  FileSystemPathParameter::ValueType OutputTrianglesFile;
  DataPath DataContainerSelection;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT WriteTriangleGeometry
{
public:
  WriteTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteTriangleGeometryInputValues* inputValues);
  ~WriteTriangleGeometry() noexcept;

  WriteTriangleGeometry(const WriteTriangleGeometry&) = delete;
  WriteTriangleGeometry(WriteTriangleGeometry&&) noexcept = delete;
  WriteTriangleGeometry& operator=(const WriteTriangleGeometry&) = delete;
  WriteTriangleGeometry& operator=(WriteTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
