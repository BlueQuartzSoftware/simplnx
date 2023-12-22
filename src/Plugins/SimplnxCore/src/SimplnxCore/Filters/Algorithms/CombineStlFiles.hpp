#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CombineStlFilesInputValues
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

class SIMPLNXCORE_EXPORT CombineStlFiles
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

} // namespace nx::core
