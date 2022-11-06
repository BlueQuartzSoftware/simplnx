#include "GeneratePrecipitateStatsData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
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
IFilter::PreflightResult GeneratePrecipitateStatsData::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pPhaseNameValue = filterArgs.value<StringParameter::ValueType>(k_PhaseName_Key);
  auto pCrystalSymmetryValue = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalSymmetry_Key);
  auto pMicroPresetModelValue = filterArgs.value<ChoicesParameter::ValueType>(k_MicroPresetModel_Key);
  auto pPhaseFractionValue = filterArgs.value<float64>(k_PhaseFraction_Key);
  auto pMuValue = filterArgs.value<float64>(k_Mu_Key);
  auto pSigmaValue = filterArgs.value<float64>(k_Sigma_Key);
  auto pMinCutOffValue = filterArgs.value<float64>(k_MinCutOff_Key);
  auto pMaxCutOffValue = filterArgs.value<float64>(k_MaxCutOff_Key);
  auto pBinStepSizeValue = filterArgs.value<float64>(k_BinStepSize_Key);
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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string numberOfBins;
  std::string featureESD;

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
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"number_of_bins", numberOfBins});
  preflightUpdatedValues.push_back({"FeatureESD", featureESD});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GeneratePrecipitateStatsData::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
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
