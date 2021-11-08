#include "GeneratePrecipitateStatsData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GeneratePrecipitateStatsData::name() const
{
  return FilterTraits<GeneratePrecipitateStatsData>::name.str();
}

//------------------------------------------------------------------------------
std::string GeneratePrecipitateStatsData::className() const
{
  return FilterTraits<GeneratePrecipitateStatsData>::className;
}

//------------------------------------------------------------------------------
Uuid GeneratePrecipitateStatsData::uuid() const
{
  return FilterTraits<GeneratePrecipitateStatsData>::uuid;
}

//------------------------------------------------------------------------------
std::string GeneratePrecipitateStatsData::humanName() const
{
  return "Generate Precipitate StatsData";
}

//------------------------------------------------------------------------------
std::vector<std::string> GeneratePrecipitateStatsData::defaultTags() const
{
  return {"#Unsupported", "#StatsGenerator"};
}

//------------------------------------------------------------------------------
Parameters GeneratePrecipitateStatsData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StringParameter>(k_PhaseName_Key, "Phase Name", "", "SomeString"));
  params.insert(std::make_unique<ChoicesParameter>(k_CrystalSymmetry_Key, "Crystal Symmetry", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ChoicesParameter>(k_MicroPresetModel_Key, "Microstructure Preset Model", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Float64Parameter>(k_PhaseFraction_Key, "Phase Fraction", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Mu_Key, "Mu", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_MinCutOff_Key, "Min.Cut Off", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_MaxCutOff_Key, "Max Cut Off", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_BinStepSize_Key, "Bin Step Size", "", 2.3456789));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_NumberOfBins_Key, "Bins Created:", "", {}));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_FeatureESD_Key, "Feature ESD:", "", {}));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_OdfData_Key, "ODF", "", {}));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_MdfData_Key, "MDF", "", {}));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_AxisOdfData_Key, "Axis ODF", "", {}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RdfMinMaxDistance_Key, "[RDF] Min/Max Distance", "", std::vector<float32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<Int32Parameter>(k_RdfNumBins_Key, "[RDF] Number of Bins", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RdfBoxSize_Key, "[RDF] Box Size (X, Y, Z)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateEnsembleAttributeMatrix_Key, "Create Data Container & Ensemble AttributeMatrix", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_AppendToExistingAttributeMatrix_Key, "Append To Existing AttributeMatrix", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedEnsembleAttributeMatrix_Key, "Selected Ensemble AttributeMatrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CreateEnsembleAttributeMatrix_Key, k_DataContainerName_Key, true);
  params.linkParameters(k_CreateEnsembleAttributeMatrix_Key, k_CellEnsembleAttributeMatrixName_Key, true);
  params.linkParameters(k_AppendToExistingAttributeMatrix_Key, k_SelectedEnsembleAttributeMatrix_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GeneratePrecipitateStatsData::clone() const
{
  return std::make_unique<GeneratePrecipitateStatsData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GeneratePrecipitateStatsData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPhaseNameValue = filterArgs.value<StringParameter::ValueType>(k_PhaseName_Key);
  auto pCrystalSymmetryValue = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalSymmetry_Key);
  auto pMicroPresetModelValue = filterArgs.value<ChoicesParameter::ValueType>(k_MicroPresetModel_Key);
  auto pPhaseFractionValue = filterArgs.value<float64>(k_PhaseFraction_Key);
  auto pMuValue = filterArgs.value<float64>(k_Mu_Key);
  auto pSigmaValue = filterArgs.value<float64>(k_Sigma_Key);
  auto pMinCutOffValue = filterArgs.value<float64>(k_MinCutOff_Key);
  auto pMaxCutOffValue = filterArgs.value<float64>(k_MaxCutOff_Key);
  auto pBinStepSizeValue = filterArgs.value<float64>(k_BinStepSize_Key);
  auto pNumberOfBinsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumberOfBins_Key);
  auto pFeatureESDValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FeatureESD_Key);
  auto pOdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OdfData_Key);
  auto pMdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MdfData_Key);
  auto pAxisOdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AxisOdfData_Key);
  auto pRdfMinMaxDistanceValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RdfMinMaxDistance_Key);
  auto pRdfNumBinsValue = filterArgs.value<int32>(k_RdfNumBins_Key);
  auto pRdfBoxSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RdfBoxSize_Key);
  auto pCreateEnsembleAttributeMatrixValue = filterArgs.value<bool>(k_CreateEnsembleAttributeMatrix_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pAppendToExistingAttributeMatrixValue = filterArgs.value<bool>(k_AppendToExistingAttributeMatrix_Key);
  auto pSelectedEnsembleAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedEnsembleAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GeneratePrecipitateStatsDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> GeneratePrecipitateStatsData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPhaseNameValue = filterArgs.value<StringParameter::ValueType>(k_PhaseName_Key);
  auto pCrystalSymmetryValue = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalSymmetry_Key);
  auto pMicroPresetModelValue = filterArgs.value<ChoicesParameter::ValueType>(k_MicroPresetModel_Key);
  auto pPhaseFractionValue = filterArgs.value<float64>(k_PhaseFraction_Key);
  auto pMuValue = filterArgs.value<float64>(k_Mu_Key);
  auto pSigmaValue = filterArgs.value<float64>(k_Sigma_Key);
  auto pMinCutOffValue = filterArgs.value<float64>(k_MinCutOff_Key);
  auto pMaxCutOffValue = filterArgs.value<float64>(k_MaxCutOff_Key);
  auto pBinStepSizeValue = filterArgs.value<float64>(k_BinStepSize_Key);
  auto pNumberOfBinsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumberOfBins_Key);
  auto pFeatureESDValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FeatureESD_Key);
  auto pOdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OdfData_Key);
  auto pMdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MdfData_Key);
  auto pAxisOdfDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AxisOdfData_Key);
  auto pRdfMinMaxDistanceValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RdfMinMaxDistance_Key);
  auto pRdfNumBinsValue = filterArgs.value<int32>(k_RdfNumBins_Key);
  auto pRdfBoxSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RdfBoxSize_Key);
  auto pCreateEnsembleAttributeMatrixValue = filterArgs.value<bool>(k_CreateEnsembleAttributeMatrix_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pAppendToExistingAttributeMatrixValue = filterArgs.value<bool>(k_AppendToExistingAttributeMatrix_Key);
  auto pSelectedEnsembleAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedEnsembleAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
