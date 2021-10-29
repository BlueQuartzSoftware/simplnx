#include "WriteASCIIData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteASCIIData::name() const
{
  return FilterTraits<WriteASCIIData>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteASCIIData::className() const
{
  return FilterTraits<WriteASCIIData>::className;
}

//------------------------------------------------------------------------------
Uuid WriteASCIIData::uuid() const
{
  return FilterTraits<WriteASCIIData>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteASCIIData::humanName() const
{
  return "Export ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteASCIIData::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters WriteASCIIData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputStyle_Key, "Output Type", "", 0, ChoicesParameter::Choices{"Multiple Files", "Single File"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_MaxValPerLine_Key, "Maximum Tuples Per Line", "", 1234356));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFilePath_Key, "Output File Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_OutputPath_Key, 0);
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, 1);
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, 2);
  params.linkParameters(k_OutputStyle_Key, k_OutputFilePath_Key, 3);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteASCIIData::clone() const
{
  return std::make_unique<WriteASCIIData>();
}

//------------------------------------------------------------------------------
Result<OutputActions> WriteASCIIData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pMaxValPerLineValue = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WriteASCIIDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> WriteASCIIData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pMaxValPerLineValue = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
