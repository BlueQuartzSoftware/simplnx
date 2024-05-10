#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/PythonPluginTemplateFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreatePythonSkeletonInputValues
{
  bool useExistingPlugin;
  std::filesystem::path pluginInputDir;
  std::filesystem::path pluginOutputDir;
  std::string pluginName;
  std::string pluginHumanName;
  std::string filterNames;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT CreatePythonSkeleton
{
public:
  CreatePythonSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreatePythonSkeletonInputValues* inputValues);
  ~CreatePythonSkeleton() noexcept;

  CreatePythonSkeleton(const CreatePythonSkeleton&) = delete;
  CreatePythonSkeleton(CreatePythonSkeleton&&) noexcept = delete;
  CreatePythonSkeleton& operator=(const CreatePythonSkeleton&) = delete;
  CreatePythonSkeleton& operator=(CreatePythonSkeleton&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreatePythonSkeletonInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
