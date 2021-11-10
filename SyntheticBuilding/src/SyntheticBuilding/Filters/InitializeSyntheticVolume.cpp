#include "InitializeSyntheticVolume.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string InitializeSyntheticVolume::name() const
{
  return FilterTraits<InitializeSyntheticVolume>::name.str();
}

//------------------------------------------------------------------------------
std::string InitializeSyntheticVolume::className() const
{
  return FilterTraits<InitializeSyntheticVolume>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeSyntheticVolume::uuid() const
{
  return FilterTraits<InitializeSyntheticVolume>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeSyntheticVolume::humanName() const
{
  return "Initialize Synthetic Volume";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeSyntheticVolume::defaultTags() const
{
  return {"#Synthetic Building", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters InitializeSyntheticVolume::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_EstimateNumberOfFeatures_Key, "Estimate Number of Features", "", false));
  params.insertSeparator(Parameters::Separator{"Geometry Selection"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_GeometryDataContainer_Key, "Existing Geometry", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Synthetic Volume Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GeometrySelection_Key, "Source of Geometry", "", 0, ChoicesParameter::Choices{"Create Geometry", "Copy Geometry"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Units (For Description Only)", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_EstimateNumberOfFeatures_Key, k_InputPhaseTypesArrayPath_Key, true);
  params.linkParameters(k_GeometrySelection_Key, k_Dimensions_Key, 0);
  params.linkParameters(k_GeometrySelection_Key, k_Spacing_Key, 0);
  params.linkParameters(k_GeometrySelection_Key, k_Origin_Key, 0);
  params.linkParameters(k_GeometrySelection_Key, k_LengthUnit_Key, 0);
  params.linkParameters(k_GeometrySelection_Key, k_GeometryDataContainer_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeSyntheticVolume::clone() const
{
  return std::make_unique<InitializeSyntheticVolume>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeSyntheticVolume::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pEstimateNumberOfFeaturesValue = filterArgs.value<bool>(k_EstimateNumberOfFeatures_Key);
  auto pGeometryDataContainerValue = filterArgs.value<DataPath>(k_GeometryDataContainer_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pGeometrySelectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GeometrySelection_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string estimatedPrimaryFeatures;
  std::string boxDimensions;

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<InitializeSyntheticVolumeAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  // These values should have been updated during the preflightImpl(...) method
  preflightResult.outputValues.push_back({"EstimatedPrimaryFeatures", estimatedPrimaryFeatures});
  preflightResult.outputValues.push_back({"BoxDimensions", boxDimensions});

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> InitializeSyntheticVolume::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pEstimateNumberOfFeaturesValue = filterArgs.value<bool>(k_EstimateNumberOfFeatures_Key);
  auto pGeometryDataContainerValue = filterArgs.value<DataPath>(k_GeometryDataContainer_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pGeometrySelectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GeometrySelection_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
