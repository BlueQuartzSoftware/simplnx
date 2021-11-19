#include "FeatureDataCSVWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::name() const
{
  return FilterTraits<FeatureDataCSVWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::className() const
{
  return FilterTraits<FeatureDataCSVWriter>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureDataCSVWriter::uuid() const
{
  return FilterTraits<FeatureDataCSVWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::humanName() const
{
  return "Export Feature Data as CSV File";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureDataCSVWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters FeatureDataCSVWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_FeatureDataFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteNeighborListData_Key, "Write Neighbor Data", "", false));
  params.insert(std::make_unique<BoolParameter>(k_WriteNumFeaturesLine_Key, "Write Number of Features Line", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceInt_Key, "Delimiter", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureDataCSVWriter::clone() const
{
  return std::make_unique<FeatureDataCSVWriter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FeatureDataCSVWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureDataFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

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
Result<> FeatureDataCSVWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureDataFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
