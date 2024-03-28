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

struct SIMPLNXCORE_EXPORT GeneratePythonSkeletonInputValues
{
  bool useExistingPlugin;
  std::filesystem::path pluginInputDir;
  std::filesystem::path pluginOutputDir;
  std::string pluginName;
  std::string pluginHumanName;
  std::string filterNames;
  bool createBatchShellScript;
  std::string anacondaEnvName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT GeneratePythonSkeleton
{
public:
  GeneratePythonSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GeneratePythonSkeletonInputValues* inputValues);
  ~GeneratePythonSkeleton() noexcept;

  GeneratePythonSkeleton(const GeneratePythonSkeleton&) = delete;
  GeneratePythonSkeleton(GeneratePythonSkeleton&&) noexcept = delete;
  GeneratePythonSkeleton& operator=(const GeneratePythonSkeleton&) = delete;
  GeneratePythonSkeleton& operator=(GeneratePythonSkeleton&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GeneratePythonSkeletonInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
