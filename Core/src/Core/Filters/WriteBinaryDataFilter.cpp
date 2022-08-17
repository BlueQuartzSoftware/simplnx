#include "WriteBinaryDataFilter.hpp"
#include "Algorithms/WriteBinaryData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
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
std::string WriteBinaryDataFilter::name() const
{
  return FilterTraits<WriteBinaryDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::className() const
{
  return FilterTraits<WriteBinaryDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteBinaryDataFilter::uuid() const
{
  return FilterTraits<WriteBinaryDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::humanName() const
{
  return "Export Binary Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteBinaryDataFilter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters WriteBinaryDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Endianess_Key, "Endianess", "Default is little endian", to_underlying(Endianess::Little),
                                                   ChoicesParameter::Choices{"Little Endian", "Big Endian"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default output directory>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "", ".bin"));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "", MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteBinaryDataFilter::clone() const
{
  return std::make_unique<WriteBinaryDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteBinaryDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pEndianessValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endianess_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
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
Result<> WriteBinaryDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  WriteBinaryDataInputValues inputValues;

  inputValues.endianess = filterArgs.value<ChoicesParameter::ValueType>(k_Endianess_Key);
  inputValues.outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.fileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  inputValues.selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  return WriteBinaryData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
