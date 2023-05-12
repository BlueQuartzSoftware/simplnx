#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT CombineStlFilesInputValues
{
  FileSystemPathParameter::ValueType StlFilesPath;
  DataPath TriangleDataContainerName;
  DataPath FaceAttributeMatrixName;
  DataPath FaceNormalsArrayName;
};

/**
 * @class CombineStlFiles
 * @brief This filter combines all of the STL files from a given directory into a single triangle geometry
 */

class COMPLEXCORE_EXPORT CombineStlFiles
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
