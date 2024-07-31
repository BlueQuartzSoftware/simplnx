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
  SingleFile,
  PartNumber
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
  DataPath PartNumberPath;
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

  void sendThreadSafeProgressMessage(Result<> result);

private:
  DataStructure& m_DataStructure;
  const WriteStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;

  mutable bool m_HasErrors = false;
  Result<> m_Result;
};

} // namespace nx::core
