#include "CreateEnsembleInfoFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/CreateEnsembleInfo.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/EnsembleInfoParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return {className(), "Processing", "Generation", "EBSD", "Phase", "Create", "Generate"};
}

//------------------------------------------------------------------------------
Parameters CreateEnsembleInfoFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<EnsembleInfoParameter>(
      k_Ensemble_Key, "Created Ensemble Info", "The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.",
      EnsembleInfoParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CellEnsembleAttributeMatrixPath_Key, "Ensemble Attribute Matrix",
                                                             "The complete path to the attribute matrix in which to store the ensemble phase data arrays", DataPath({"EnsembleAttributeMatrix"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "The name of the data array representing the crystal structure for each Ensemble",
                                                          "CrystalStructures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "The name of the data array representing the phase types for each Ensemble", "PhaseTypes"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseNamesArrayName_Key, "Phase Names", "The name of the string array representing the phase names for each Ensemble", "PhaseNames"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateEnsembleInfoFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType CreateEnsembleInfoFilter::clone() const
{
  return std::make_unique<CreateEnsembleInfoFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateEnsembleInfoFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                 const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pEnsembleValue = filterArgs.value<EnsembleInfoParameter::ValueType>(k_Ensemble_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseNamesArrayName_Key);

  PreflightResult preflightResult;
  OutputActions outputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  int numPhases = pEnsembleValue.size();
  ShapeType tDims(1, numPhases + 1);
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
Result<> CreateEnsembleInfoFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  CreateEnsembleInfoInputValues inputValues;
  inputValues.ensemble = filterArgs.value<EnsembleInfoParameter::ValueType>(k_Ensemble_Key);
  inputValues.cell_ensemble_attribute_matrix_name = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  inputValues.crystal_structures_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_CrystalStructuresArrayName_Key);
  inputValues.phase_types_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseTypesArrayName_Key);
  inputValues.phase_names_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_PhaseNamesArrayName_Key);

  return CreateEnsembleInfo(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteralType k_EnsembleKey = "Ensemble";
constexpr StringLiteralType k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
constexpr StringLiteralType k_CrystalStructuresArrayNameKey = "CrystalStructuresArrayName";
constexpr StringLiteralType k_PhaseTypesArrayNameKey = "PhaseTypesArrayName";
constexpr StringLiteralType k_PhaseNamesArrayNameKey = "PhaseNamesArrayName";
} // namespace simpl
} // namespace

Result<Arguments> CreateEnsembleInfoFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateEnsembleInfoFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Data Container Name parameter is not applicable in NX

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::EnsembleInfoFilterParameterConverter>(args, json, simpl::k_EnsembleKey, k_Ensemble_Key));
  results.push_back(simpl_conversion::Convert2Parameters<simpl_conversion::AMPathBuilderFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey,
                                                                                                                  simpl::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayNameKey, k_CrystalStructuresArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_PhaseTypesArrayNameKey, k_PhaseTypesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_PhaseNamesArrayNameKey, k_PhaseNamesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
