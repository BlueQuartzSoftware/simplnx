#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

enum class GroupingType : ChoicesParameter::ValueType
{
  Features,
  FeaturesAndPhases,
  None
};

struct SIMPLNXCORE_EXPORT WriteStlFileInputValues
{
  ChoicesParameter::ValueType GroupingType;
  FileSystemPathParameter::ValueType OutputStlFile;
  FileSystemPathParameter::ValueType OutputStlDirectory;
  StringParameter::ValueType OutputStlPrefix;
  DataPath FeatureIdsPath;
  DataPath FeaturePhasesPath;
  DataPath TriangleGeomPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT WriteStlFile
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

} // namespace nx::core
