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
//------------------------------------------------------------------------------
std::string EstablishFoamMorphology::name() const
{
  return FilterTraits<EstablishFoamMorphology>::name.str();
}

//------------------------------------------------------------------------------
std::string EstablishFoamMorphology::className() const
{
  return FilterTraits<EstablishFoamMorphology>::className;
}

//------------------------------------------------------------------------------
Uuid EstablishFoamMorphology::uuid() const
{
  return FilterTraits<EstablishFoamMorphology>::uuid;
}

//------------------------------------------------------------------------------
std::string EstablishFoamMorphology::humanName() const
{
  return "Establish Foam Morphology";
}

//------------------------------------------------------------------------------
std::vector<std::string> EstablishFoamMorphology::defaultTags() const
{
  return {"#Synthetic Building", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters EstablishFoamMorphology::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_PeriodicBoundaries_Key, "Periodic Boundaries", "", false));
  params.insertSeparator(Parameters::Separator{"Data Container"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputShapeTypesArrayPath_Key, "Shape Types", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputCellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputCellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_NumFeaturesArrayName_Key, "Number of Features", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "Specifies to which Feature each Element belongs", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MaskArrayName_Key, "Mask", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellPhasesArrayName_Key, "Phases", "Specifies to which Ensemble each cell belongs", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_QPEuclideanDistancesArrayName_Key, "Quadruple Point Euclidean Distances", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TJEuclideanDistancesArrayName_Key, "Triple Junction Euclidean Distances", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_GBEuclideanDistancesArrayName_Key, "Grain Boundary Euclidean Distances", "", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteGoalAttributes_Key, "Write Goal Attributes", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_CsvOutputFile_Key, "Goal Attribute CSV File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));
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

//------------------------------------------------------------------------------
IFilter::UniquePointer EstablishFoamMorphology::clone() const
{
  return std::make_unique<EstablishFoamMorphology>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EstablishFoamMorphology::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> EstablishFoamMorphology::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
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
