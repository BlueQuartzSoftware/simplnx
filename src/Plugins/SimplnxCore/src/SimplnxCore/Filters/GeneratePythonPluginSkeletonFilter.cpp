#include "GeneratePythonPluginSkeletonFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/GeneratePythonPluginSkeleton.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string GeneratePythonPluginSkeletonFilter::name() const
{
  return FilterTraits<GeneratePythonPluginSkeletonFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GeneratePythonPluginSkeletonFilter::className() const
{
  return FilterTraits<GeneratePythonPluginSkeletonFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GeneratePythonPluginSkeletonFilter::uuid() const
{
  return FilterTraits<GeneratePythonPluginSkeletonFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GeneratePythonPluginSkeletonFilter::humanName() const
{
  return "Generate Python Plugin Skeleton";
}

//------------------------------------------------------------------------------
std::vector<std::string> GeneratePythonPluginSkeletonFilter::defaultTags() const
{
  return {className(), "Generic", "Python",    "Plugin", "Skeleton", "Generate", "Create", "Template", "Code",  "Produce",
          "Form",      "Develop", "Construct", "Make",   "Build",    "Engineer", "Invent", "Initiate", "Design"};
}

//------------------------------------------------------------------------------
Parameters GeneratePythonPluginSkeletonFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<StringParameter>(k_PluginName_Key, "Name of Plugin", "This is the name of the plugin.", "ExamplePlugin"));
  params.insert(std::make_unique<StringParameter>(k_PluginHumanName_Key, "Human Name of Plugin", "This is the user facing name of the plugin.", "ExamplePlugin"));

  params.insert(std::make_unique<FileSystemPathParameter>(k_PluginOutputDirectory_Key, "Output Directory", "", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));

  params.insert(std::make_unique<StringParameter>(k_PluginFilterNames, "List of names of filters", "Separate with commas (,)", "FirstFilter,SecondFilter"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GeneratePythonPluginSkeletonFilter::clone() const
{
  return std::make_unique<GeneratePythonPluginSkeletonFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GeneratePythonPluginSkeletonFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{

  auto pluginOutputDir = filterArgs.value<FileSystemPathParameter::ValueType>(k_PluginOutputDirectory_Key);
  auto pluginName = filterArgs.value<StringParameter::ValueType>(k_PluginName_Key);
  auto pluginHumanName = filterArgs.value<StringParameter::ValueType>(k_PluginHumanName_Key);

  auto pluginFilters = filterArgs.value<StringParameter::ValueType>(k_PluginFilterNames);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  preflightUpdatedValues.push_back({"Generated Plugin Directory", fmt::format("", pluginOutputDir.string(), fs::path::preferred_separator, pluginName)});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GeneratePythonPluginSkeletonFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  GeneratePythonPluginSkeletonInputValues inputValues;

  inputValues.pluginOutputDir = filterArgs.value<FileSystemPathParameter::ValueType>(k_PluginOutputDirectory_Key);
  inputValues.pluginName = filterArgs.value<StringParameter::ValueType>(k_PluginName_Key);
  inputValues.pluginHumanName = filterArgs.value<StringParameter::ValueType>(k_PluginHumanName_Key);
  inputValues.filterNames = filterArgs.value<StringParameter::ValueType>(k_PluginFilterNames);

  return GeneratePythonPluginSkeleton(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

} // namespace nx::core
