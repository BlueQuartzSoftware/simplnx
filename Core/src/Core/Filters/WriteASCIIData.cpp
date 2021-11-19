#include "WriteASCIIData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, 0);
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, 0);
  params.linkParameters(k_OutputStyle_Key, k_OutputFilePath_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteASCIIData::clone() const
{
  return std::make_unique<WriteASCIIData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteASCIIData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pMaxValPerLineValue = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
