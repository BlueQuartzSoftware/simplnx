#include "CreateEnsembleInfoFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/CreateEnsembleInfo.hpp"

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/EnsembleInfoParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateEnsembleInfoFilter::name() const
{
  return FilterTraits<CreateEnsembleInfoFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateEnsembleInfoFilter::className() const
{
  return FilterTraits<CreateEnsembleInfoFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateEnsembleInfoFilter::uuid() const
{
  return FilterTraits<CreateEnsembleInfoFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateEnsembleInfoFilter::humanName() const
{
  return "Create Ensemble Info";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateEnsembleInfoFilter::defaultTags() const
{
  return {className(), "Processing", "Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateEnsembleInfoFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<EnsembleInfoParameter>(
      k_Ensemble_Key, "Created Ensemble Info", "The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.",
      EnsembleInfoParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Created Cell Ensemble Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix",
                                                             "The complete path to the attribute matrix in which to store the ensemble phase data arrays", DataPath({"EnsembleAttributeMatrix"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "The name of the data array representing the crystal structure for each Ensemble",
                                                          "CrystalStructures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "The name of the data array representing the phase types for each Ensemble", "PhaseTypes"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseNamesArrayName_Key, "Phase Names", "The name of the string array representing the phase names for each Ensemble", "PhaseNames"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateEnsembleInfoFilter::clone() const
{
  return std::make_unique<CreateEnsembleInfoFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateEnsembleInfoFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pEnsembleValue = filterArgs.value<EnsembleInfoParameter::ValueType>(k_Ensemble_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseNamesArrayName_Key);

  PreflightResult preflightResult;
  OutputActions outputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  int numPhases = pEnsembleValue.size();
  std::vector<usize> tDims(1, numPhases + 1);
  auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(pCellEnsembleAttributeMatrixNameValue, tDims);
  outputActions.appendAction(std::move(createAttributeMatrixAction));

  std::vector<size_t> cDims(1, 1);
  auto createCrystalStructuresAction = std::make_unique<CreateArrayAction>(DataType::uint32, tDims, cDims, pCellEnsembleAttributeMatrixNameValue.createChildPath(pCrystalStructuresArrayNameValue));
  auto createPhaseTypesAction = std::make_unique<CreateArrayAction>(DataType::uint32, tDims, cDims, pCellEnsembleAttributeMatrixNameValue.createChildPath(pPhaseTypesArrayNameValue));
  auto createPhaseNamesAction =
      std::make_unique<CreateStringArrayAction>(tDims, pCellEnsembleAttributeMatrixNameValue.createChildPath(pPhaseNamesArrayNameValue), EnsembleInfoParameter::k_DefaultPhaseName);
  outputActions.appendAction(std::move(createCrystalStructuresAction));
  outputActions.appendAction(std::move(createPhaseTypesAction));
  outputActions.appendAction(std::move(createPhaseNamesAction));

  return {std::move(outputActions)};
}

//------------------------------------------------------------------------------
Result<> CreateEnsembleInfoFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  CreateEnsembleInfoInputValues inputValues;
  inputValues.Ensemble = filterArgs.value<EnsembleInfoParameter::ValueType>(k_Ensemble_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.CrystalStructuresArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CrystalStructuresArrayName_Key);
  inputValues.PhaseTypesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseTypesArrayName_Key);
  inputValues.PhaseNamesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseNamesArrayName_Key);

  return CreateEnsembleInfo(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
