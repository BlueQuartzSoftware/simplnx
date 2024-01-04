#include "GeneratePythonSkeletonFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/GeneratePythonSkeleton.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;


namespace nx::core
{
//------------------------------------------------------------------------------
std::string GeneratePythonSkeletonFilter::name() const
{
  return FilterTraits<GeneratePythonSkeletonFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GeneratePythonSkeletonFilter::className() const
{
  return FilterTraits<GeneratePythonSkeletonFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GeneratePythonSkeletonFilter::uuid() const
{
  return FilterTraits<GeneratePythonSkeletonFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GeneratePythonSkeletonFilter::humanName() const
{
  return "Generate Python Plugin Skeleton";
}

//------------------------------------------------------------------------------
std::vector<std::string> GeneratePythonSkeletonFilter::defaultTags() const
{
  return {className(), "Generic", "Python", "Code"};
}

//------------------------------------------------------------------------------
Parameters GeneratePythonSkeletonFilter::parameters() const
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
IFilter::UniquePointer GeneratePythonSkeletonFilter::clone() const
{
  return std::make_unique<GeneratePythonSkeletonFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GeneratePythonSkeletonFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> GeneratePythonSkeletonFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  GeneratePythonSkeletonInputValues inputValues;

  inputValues.pluginOutputDir = filterArgs.value<FileSystemPathParameter::ValueType>(k_PluginOutputDirectory_Key);
  inputValues.pluginName = filterArgs.value<StringParameter::ValueType>(k_PluginName_Key);
  inputValues.pluginHumanName = filterArgs.value<StringParameter::ValueType>(k_PluginHumanName_Key);
  inputValues.filterNames = filterArgs.value<StringParameter::ValueType>(k_PluginFilterNames);

  return GeneratePythonSkeleton(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

} // namespace nx::cor
