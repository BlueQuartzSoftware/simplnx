#include "PackPrimaryPhases.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
std::string PackPrimaryPhases::name() const
{
  return FilterTraits<PackPrimaryPhases>::name.str();
}

Uuid PackPrimaryPhases::uuid() const
{
  return FilterTraits<PackPrimaryPhases>::uuid;
}

std::string PackPrimaryPhases::humanName() const
{
  return "Pack Primary Phases";
}

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
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_NewAttributeMatrixPath_Key, 0);
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_SelectedAttributeMatrixPath_Key, 1);

  return params;
}

IFilter::UniquePointer PackPrimaryPhases::clone() const
{
  return std::make_unique<PackPrimaryPhases>();
}

Result<OutputActions> PackPrimaryPhases::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<PackPrimaryPhasesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> PackPrimaryPhases::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
