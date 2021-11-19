#include "FeatureInfoReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
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
std::string FeatureInfoReader::name() const
{
  return FilterTraits<FeatureInfoReader>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureInfoReader::className() const
{
  return FilterTraits<FeatureInfoReader>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureInfoReader::uuid() const
{
  return FilterTraits<FeatureInfoReader>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureInfoReader::humanName() const
{
  return "Import Feature Info File";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureInfoReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters FeatureInfoReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input Feature Info File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateCellLevelArrays_Key, "Create Element Level Arrays", "", false));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "", false));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixName_Key, "Element Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellPhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEulerAnglesArrayName_Key, "Euler Angles", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureEulerAnglesArrayName_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CreateCellLevelArrays_Key, k_CellPhasesArrayName_Key, true);
  params.linkParameters(k_CreateCellLevelArrays_Key, k_CellEulerAnglesArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureInfoReader::clone() const
{
  return std::make_unique<FeatureInfoReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FeatureInfoReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pCreateCellLevelArraysValue = filterArgs.value<bool>(k_CreateCellLevelArrays_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);

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
Result<> FeatureInfoReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pCreateCellLevelArraysValue = filterArgs.value<bool>(k_CreateCellLevelArrays_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
