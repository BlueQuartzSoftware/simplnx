#include "AxioVisionV4ToTileConfiguration.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AxioVisionV4ToTileConfiguration::name() const
{
  return FilterTraits<AxioVisionV4ToTileConfiguration>::name.str();
}

//------------------------------------------------------------------------------
std::string AxioVisionV4ToTileConfiguration::className() const
{
  return FilterTraits<AxioVisionV4ToTileConfiguration>::className;
}

//------------------------------------------------------------------------------
Uuid AxioVisionV4ToTileConfiguration::uuid() const
{
  return FilterTraits<AxioVisionV4ToTileConfiguration>::uuid;
}

//------------------------------------------------------------------------------
std::string AxioVisionV4ToTileConfiguration::humanName() const
{
  return "ITK::Convert AxioVision To Tile Configuration file";
}

//------------------------------------------------------------------------------
std::vector<std::string> AxioVisionV4ToTileConfiguration::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters AxioVisionV4ToTileConfiguration::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AxioVisionV4ToTileConfiguration::clone() const
{
  return std::make_unique<AxioVisionV4ToTileConfiguration>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AxioVisionV4ToTileConfiguration::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AxioVisionV4ToTileConfigurationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AxioVisionV4ToTileConfiguration::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
