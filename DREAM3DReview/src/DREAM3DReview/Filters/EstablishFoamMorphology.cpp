#include "EstablishFoamMorphology.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string EstablishFoamMorphology::name() const
{
  return FilterTraits<EstablishFoamMorphology>::name.str();
}

std::string EstablishFoamMorphology::className() const
{
  return FilterTraits<EstablishFoamMorphology>::className;
}

Uuid EstablishFoamMorphology::uuid() const
{
  return FilterTraits<EstablishFoamMorphology>::uuid;
}

std::string EstablishFoamMorphology::humanName() const
{
  return "Establish Foam Morphology";
}

Parameters EstablishFoamMorphology::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_PeriodicBoundaries_Key, "Periodic Boundaries", "", false));
  params.insertSeparator(Parameters::Separator{"Data Container"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputShapeTypesArrayPath_Key, "Shape Types", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputCellFeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputCellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_NumFeaturesArrayName_Key, "Number of Features", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MaskArrayName_Key, "Mask", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellPhasesArrayName_Key, "Phases", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_QPEuclideanDistancesArrayName_Key, "Quadruple Point Euclidean Distances", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TJEuclideanDistancesArrayName_Key, "Triple Junction Euclidean Distances", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_GBEuclideanDistancesArrayName_Key, "Grain Boundary Euclidean Distances", "", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteGoalAttributes_Key, "Write Goal Attributes", "", false));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_CsvOutputFile_Key, "Goal Attribute CSV File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_HaveFeatures_Key, "Already Have Features", "", 0, ChoicesParameter::Choices{"No", "Yes"}));
  params.insert(std::make_unique<Float64Parameter>(k_MinStrutThickness_Key, "Minimum Strut Thickness", "", 2.3456789));
  params.insert(std::make_unique<Float32Parameter>(k_StrutThicknessVariability_Key, "Strut Thickness Variability Factor", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_StrutShapeVariability_Key, "Strut Cross Section Shape Factor", "", 1.23345f));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WriteGoalAttributes_Key, k_CsvOutputFile_Key, true);
  params.linkParameters(k_WriteGoalAttributes_Key, k_CsvOutputFile_Key, true);
  params.linkParameters(k_HaveFeatures_Key, k_FeatureIdsArrayName_Key, 0);
  params.linkParameters(k_HaveFeatures_Key, k_InputCellFeatureIdsArrayPath_Key, 1);

  return params;
}

IFilter::UniquePointer EstablishFoamMorphology::clone() const
{
  return std::make_unique<EstablishFoamMorphology>();
}

Result<OutputActions> EstablishFoamMorphology::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pInputCellFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_InputCellFeatureIdsArrayPath_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NumFeaturesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_FeatureIdsArrayName_Key);
  auto pMaskArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CellPhasesArrayName_Key);
  auto pQPEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_QPEuclideanDistancesArrayName_Key);
  auto pTJEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TJEuclideanDistancesArrayName_Key);
  auto pGBEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_GBEuclideanDistancesArrayName_Key);
  auto pWriteGoalAttributesValue = filterArgs.value<bool>(k_WriteGoalAttributes_Key);
  auto pCsvOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CsvOutputFile_Key);
  auto pHaveFeaturesValue = filterArgs.value<ChoicesParameter::ValueType>(k_HaveFeatures_Key);
  auto pMinStrutThicknessValue = filterArgs.value<float64>(k_MinStrutThickness_Key);
  auto pStrutThicknessVariabilityValue = filterArgs.value<float32>(k_StrutThicknessVariability_Key);
  auto pStrutShapeVariabilityValue = filterArgs.value<float32>(k_StrutShapeVariability_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<EstablishFoamMorphologyAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> EstablishFoamMorphology::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPeriodicBoundariesValue = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pInputShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  auto pInputCellFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_InputCellFeatureIdsArrayPath_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NumFeaturesArrayName_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_FeatureIdsArrayName_Key);
  auto pMaskArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CellPhasesArrayName_Key);
  auto pQPEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_QPEuclideanDistancesArrayName_Key);
  auto pTJEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TJEuclideanDistancesArrayName_Key);
  auto pGBEuclideanDistancesArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_GBEuclideanDistancesArrayName_Key);
  auto pWriteGoalAttributesValue = filterArgs.value<bool>(k_WriteGoalAttributes_Key);
  auto pCsvOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CsvOutputFile_Key);
  auto pHaveFeaturesValue = filterArgs.value<ChoicesParameter::ValueType>(k_HaveFeatures_Key);
  auto pMinStrutThicknessValue = filterArgs.value<float64>(k_MinStrutThickness_Key);
  auto pStrutThicknessVariabilityValue = filterArgs.value<float32>(k_StrutThicknessVariability_Key);
  auto pStrutShapeVariabilityValue = filterArgs.value<float32>(k_StrutShapeVariability_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
