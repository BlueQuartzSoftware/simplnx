#include "TesselateFarFieldGrains.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string TesselateFarFieldGrains::name() const
{
  return FilterTraits<TesselateFarFieldGrains>::name.str();
}

//------------------------------------------------------------------------------
std::string TesselateFarFieldGrains::className() const
{
  return FilterTraits<TesselateFarFieldGrains>::className;
}

//------------------------------------------------------------------------------
Uuid TesselateFarFieldGrains::uuid() const
{
  return FilterTraits<TesselateFarFieldGrains>::uuid;
}

//------------------------------------------------------------------------------
std::string TesselateFarFieldGrains::humanName() const
{
  return "Tesselate Far Field Grains";
}

//------------------------------------------------------------------------------
std::vector<std::string> TesselateFarFieldGrains::defaultTags() const
{
  return {"#Unsupported", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters TesselateFarFieldGrains::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeneratedFileListParameter>(k_FeatureInputFileListInfo_Key, "Feature Input File List", "", GeneratedFileListParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_OutputCellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellPhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureEulerAnglesArrayName_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ElasticStrainsArrayName_Key, "Elastic Strains", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TesselateFarFieldGrains::clone() const
{
  return std::make_unique<TesselateFarFieldGrains>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TesselateFarFieldGrains::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureInputFileListInfoValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_FeatureInputFileListInfo_Key);
  auto pOutputCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pElasticStrainsArrayNameValue = filterArgs.value<DataPath>(k_ElasticStrainsArrayName_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);

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
  auto action = std::make_unique<TesselateFarFieldGrainsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> TesselateFarFieldGrains::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureInputFileListInfoValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_FeatureInputFileListInfo_Key);
  auto pOutputCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pFeatureEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  auto pElasticStrainsArrayNameValue = filterArgs.value<DataPath>(k_ElasticStrainsArrayName_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
