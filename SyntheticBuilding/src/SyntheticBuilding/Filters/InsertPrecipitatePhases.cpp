#include "InsertPrecipitatePhases.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
std::string InsertPrecipitatePhases::name() const
{
  return FilterTraits<InsertPrecipitatePhases>::name.str();
}

//------------------------------------------------------------------------------
std::string InsertPrecipitatePhases::className() const
{
  return FilterTraits<InsertPrecipitatePhases>::className;
}

//------------------------------------------------------------------------------
Uuid InsertPrecipitatePhases::uuid() const
{
  return FilterTraits<InsertPrecipitatePhases>::uuid;
}

//------------------------------------------------------------------------------
std::string InsertPrecipitatePhases::humanName() const
{
  return "Insert Precipitate Phases";
}

//------------------------------------------------------------------------------
std::vector<std::string> InsertPrecipitatePhases::defaultTags() const
{
  return {"#Synthetic Building", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters InsertPrecipitatePhases::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_PeriodicBoundaries_Key, "Periodic Boundaries", "", false));
  params.insert(std::make_unique<BoolParameter>(k_MatchRDF_Key, "Match Radial Distribution Function", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Boundary Cells", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputShapeTypesArrayPath_Key, "Shape Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "", DataPath{}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_FeatureGeneration_Key, "Precipitate Generation", "", 0, ChoicesParameter::Choices{"Generate Precipitates", "Already Have Precipitates"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_PrecipInputFile_Key, "Precipitates Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SaveGeometricDescriptions_Key, "Save Shape Description Arrays", "", 0,
                                                                    ChoicesParameter::Choices{"Do Not Save", "Save To New Attribute Matrix", "Append To Existing Attribute Matrix"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewAttributeMatrixPath_Key, "New Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Selected Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_FeatureGeneration_Key, k_InputStatsArrayPath_Key, 0);
  params.linkParameters(k_FeatureGeneration_Key, k_PrecipInputFile_Key, 1);
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_NewAttributeMatrixPath_Key, 0);
  params.linkParameters(k_SaveGeometricDescriptions_Key, k_SelectedAttributeMatrixPath_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InsertPrecipitatePhases::clone() const
{
  return std::make_unique<InsertPrecipitatePhases>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InsertPrecipitatePhases::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pMatchRDFValue = filterArgs.value<bool>(k_MatchRDF_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pFeatureGenerationValue = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  auto pPrecipInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_PrecipInputFile_Key);
  auto pSaveGeometricDescriptionsValue = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  auto pNewAttributeMatrixPathValue = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<InsertPrecipitatePhasesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InsertPrecipitatePhases::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pMatchRDFValue = filterArgs.value<bool>(k_MatchRDF_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pFeatureGenerationValue = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  auto pPrecipInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_PrecipInputFile_Key);
  auto pSaveGeometricDescriptionsValue = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  auto pNewAttributeMatrixPathValue = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
