#include "ReadEnsembleInfoFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadEnsembleInfo.hpp"
#include "OrientationAnalysis/utilities/inicpp.h"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadEnsembleInfoFilter::name() const
{
  return FilterTraits<ReadEnsembleInfoFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadEnsembleInfoFilter::className() const
{
  return FilterTraits<ReadEnsembleInfoFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadEnsembleInfoFilter::uuid() const
{
  return FilterTraits<ReadEnsembleInfoFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadEnsembleInfoFilter::humanName() const
{
  return "Read Ensemble Info File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadEnsembleInfoFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadEnsembleInfoFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input Ensemble Info File", "The path to the ini formatted input file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".ini", ".txt"}, FileSystemPathParameter::PathType::InputFile));
  DataGroupSelectionParameter::AllowedTypes allowedGroupTypes = BaseGroup::GetAllGeometryGroupTypes();
  allowedGroupTypes.insert(BaseGroup::GroupType::DataGroup);
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ParentGroupPath_Key, "Data Container", "The path to the data object in which the ensemble information will be stored", DataPath{},
                                                              allowedGroupTypes));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "The name of the created Ensemble Attribute Matrix", "EnsembleAttributeMatrix"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "The name of the created array representing the crystal structure for each Ensemble",
                                                          "CrystalStructures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "The name of the created array representing the phase type for each Ensemble", "PhaseTypes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadEnsembleInfoFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadEnsembleInfoFilter::clone() const
{
  return std::make_unique<ReadEnsembleInfoFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadEnsembleInfoFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key).string();
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_ParentGroupPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<std::string>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<std::string>(k_PhaseTypesArrayName_Key);

  const DataPath cellEnsembleAttributeMatrixPath = pDataContainerNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  const DataPath crystalStructuresPath = cellEnsembleAttributeMatrixPath.createChildPath(pCrystalStructuresArrayNameValue);
  const DataPath phaseTypesPath = cellEnsembleAttributeMatrixPath.createChildPath(pPhaseTypesArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  int32 numPhases = 0;

  ini::IniFile inputIni;
  inputIni.load(pInputFileValue);
  if(inputIni.count(ReadEnsembleInfo::k_EnsembleInfo) == 0)
  {
    return MakePreflightErrorResult(-13001, fmt::format("Could not find the group name {} from the input file '{}'", ReadEnsembleInfo::k_EnsembleInfo, pInputFileValue));
  }
  if(inputIni[ReadEnsembleInfo::k_EnsembleInfo].count(ReadEnsembleInfo::k_NumberPhases) == 0)
  {
    return MakePreflightErrorResult(
        -13002, fmt::format("Could not find the key {} in the {} group from the input file '{}'", ReadEnsembleInfo::k_NumberPhases, ReadEnsembleInfo::k_EnsembleInfo, pInputFileValue));
  }
  numPhases = inputIni[ReadEnsembleInfo::k_EnsembleInfo][ReadEnsembleInfo::k_NumberPhases].as<int32>(); // read number of phases from input file

  if(0 == numPhases) // 0 was entered as the Number_Phases
  {
    return MakePreflightErrorResult(-13003, fmt::format("The {} in the {} group must be greater than 0", ReadEnsembleInfo::k_NumberPhases, ReadEnsembleInfo::k_EnsembleInfo));
  }

  std::vector<usize> tupleDims(1, numPhases + 1);
  auto attributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellEnsembleAttributeMatrixPath, tupleDims);
  resultOutputActions.value().appendAction(std::move(attributeMatrixAction));

  auto crystalStructuresAction = std::make_unique<CreateArrayAction>(DataType::uint32, tupleDims, std::vector<usize>{1}, crystalStructuresPath);
  resultOutputActions.value().appendAction(std::move(crystalStructuresAction));

  auto phaseTypesAction = std::make_unique<CreateArrayAction>(DataType::uint32, tupleDims, std::vector<usize>{1}, phaseTypesPath);
  resultOutputActions.value().appendAction(std::move(phaseTypesAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadEnsembleInfoFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  ReadEnsembleInfoInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key).string();
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_ParentGroupPath_Key);
  inputValues.CellEnsembleAttributeMatrixName = inputValues.DataContainerName.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CrystalStructuresArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_CrystalStructuresArrayName_Key));
  inputValues.PhaseTypesArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_PhaseTypesArrayName_Key));

  return ReadEnsembleInfo(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
constexpr StringLiteral k_CrystalStructuresArrayNameKey = "CrystalStructuresArrayName";
constexpr StringLiteral k_PhaseTypesArrayNameKey = "PhaseTypesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadEnsembleInfoFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadEnsembleInfoFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ParentGroupPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAttributeMatrixName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayNameKey, k_CrystalStructuresArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_PhaseTypesArrayNameKey, k_PhaseTypesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
