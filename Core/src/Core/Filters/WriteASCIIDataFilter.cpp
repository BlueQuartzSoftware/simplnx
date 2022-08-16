#include "WriteASCIIDataFilter.hpp"
#include "Algorithms/WriteASCIIData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteASCIIDataFilter::name() const
{
  return FilterTraits<WriteASCIIDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteASCIIDataFilter::className() const
{
  return FilterTraits<WriteASCIIDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteASCIIDataFilter::uuid() const
{
  return FilterTraits<WriteASCIIDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteASCIIDataFilter::humanName() const
{
  return "Export ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteASCIIDataFilter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters WriteASCIIDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputStyle_Key, "Output Type", "", to_underlying(OutputStyle::MultipleFiles),
                                                                    ChoicesParameter::Choices{"Multiple Files", "Single File"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default output directory>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "", ".txt"));
  params.insert(std::make_unique<Int32Parameter>(k_MaxValPerLine_Key, "Maximum Tuples Per Line", "", 1));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "Default Delimiter is Space", to_underlying(Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "", MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteASCIIDataFilter::clone() const
{
  return std::make_unique<WriteASCIIDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteASCIIDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pMaxValPerLineValue = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;

  // If structural changes to the DataStructure this will store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteASCIIDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  WriteASCIIDataInputValues inputValues;

  inputValues.outputStyle = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  inputValues.outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.fileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  inputValues.maxValPerLine = filterArgs.value<int32>(k_MaxValPerLine_Key);
  inputValues.delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  return WriteASCIIData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
