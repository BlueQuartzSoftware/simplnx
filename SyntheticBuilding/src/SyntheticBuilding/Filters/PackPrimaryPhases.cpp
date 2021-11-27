#include "PackPrimaryPhases.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PackPrimaryPhases::name() const
{
  return FilterTraits<PackPrimaryPhases>::name.str();
}

//------------------------------------------------------------------------------
std::string PackPrimaryPhases::className() const
{
  return FilterTraits<PackPrimaryPhases>::className;
}

//------------------------------------------------------------------------------
Uuid PackPrimaryPhases::uuid() const
{
  return FilterTraits<PackPrimaryPhases>::uuid;
}

//------------------------------------------------------------------------------
std::string PackPrimaryPhases::humanName() const
{
  return "Pack Primary Phases";
}

//------------------------------------------------------------------------------
std::vector<std::string> PackPrimaryPhases::defaultTags() const
{
  return {"#Synthetic Building", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters PackPrimaryPhases::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_PeriodicBoundaries_Key, "Periodic Boundaries", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_OutputCellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputShapeTypesArrayPath_Key, "Shape Types", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellPhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumFeaturesArrayName_Key, "Number of Features", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_FeatureGeneration_Key, "Feature Generation", "", 0, ChoicesParameter::Choices{"Generate Features", "Already Have Features"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FeatureInputFile_Key, "Feature Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SaveGeometricDescriptions_Key, "Save Shape Description Arrays", "", 0,
                                                                    ChoicesParameter::Choices{"Do Not Save", "Save To New Attribute Matrix", "Append To Existing Attribute Matrix"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewAttributeMatrixPath_Key, "New Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Selected Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_FeatureGeneration_Key, k_InputStatsArrayPath_Key, 0);
  params.linkParameters(k_FeatureGeneration_Key, k_FeatureInputFile_Key, 1);
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_NewAttributeMatrixPath_Key, 1);
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_SelectedAttributeMatrixPath_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PackPrimaryPhases::clone() const
{
  return std::make_unique<PackPrimaryPhases>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PackPrimaryPhases::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pOutputCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);
  auto pFeatureGenerationValue = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  auto pFeatureInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureInputFile_Key);
  auto pSaveGeometricDescriptionsValue = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  auto pNewAttributeMatrixPathValue = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

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
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewAttributeMatrixPathValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PackPrimaryPhases::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pOutputCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);
  auto pFeatureGenerationValue = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  auto pFeatureInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureInputFile_Key);
  auto pSaveGeometricDescriptionsValue = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  auto pNewAttributeMatrixPathValue = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
