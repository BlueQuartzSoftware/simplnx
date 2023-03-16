#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT WriteStlFileInputValues
{
  FileSystemPathParameter::ValueType OutputStlDirectory;
  StringParameter::ValueType OutputStlPrefix;
  DataPath SurfaceMeshFaceLabelsArrayPath;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT WriteStlFile
{
public:
  WriteStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStlFileInputValues* inputValues);
  ~WriteStlFile() noexcept;

  WriteStlFile(const WriteStlFile&) = delete;
  WriteStlFile(WriteStlFile&&) noexcept = delete;
  WriteStlFile& operator=(const WriteStlFile&) = delete;
  WriteStlFile& operator=(WriteStlFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
