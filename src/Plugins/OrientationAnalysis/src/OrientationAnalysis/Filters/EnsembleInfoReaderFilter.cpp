#include "EnsembleInfoReaderFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/EnsembleInfoReader.hpp"
#include "OrientationAnalysis/utilities/inicpp.h"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EnsembleInfoReaderFilter::name() const
{
  return FilterTraits<EnsembleInfoReaderFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string EnsembleInfoReaderFilter::className() const
{
  return FilterTraits<EnsembleInfoReaderFilter>::className;
}

//------------------------------------------------------------------------------
Uuid EnsembleInfoReaderFilter::uuid() const
{
  return FilterTraits<EnsembleInfoReaderFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string EnsembleInfoReaderFilter::humanName() const
{
  return "Import Ensemble Info File";
}

//------------------------------------------------------------------------------
std::vector<std::string> EnsembleInfoReaderFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters EnsembleInfoReaderFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input Ensemble Info File", "The path to the ini formatted input file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".ini", ".txt"}, FileSystemPathParameter::PathType::InputFile));
  DataGroupSelectionParameter::AllowedTypes allowedGroupTypes = BaseGroup::GetAllGeometryGroupTypes();
  allowedGroupTypes.insert(BaseGroup::GroupType::DataGroup);
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "The path to the data object in which the ensemble information will be stored", DataPath{},
                                                              allowedGroupTypes));
  params.insertSeparator(Parameters::Separator{"Created Ensemble Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "The name of the created Ensemble Attribute Matrix", "EnsembleAttributeMatrix"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "The name of the created array representing the crystal structure for each Ensemble",
                                                          "CrystalStructures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "The name of the created array representing the phase type for each Ensemble", "PhaseTypes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EnsembleInfoReaderFilter::clone() const
{
  return std::make_unique<EnsembleInfoReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EnsembleInfoReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key).string();
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<std::string>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<std::string>(k_PhaseTypesArrayName_Key);

  const DataPath cellEnsembleAttributeMatrixPath = pDataContainerNameValue.createChildPath(pCellEnsembleAttributeMatrixNameValue);
  const DataPath crystalStructuresPath = cellEnsembleAttributeMatrixPath.createChildPath(pCrystalStructuresArrayNameValue);
  const DataPath phaseTypesPath = cellEnsembleAttributeMatrixPath.createChildPath(pPhaseTypesArrayNameValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  int32 numPhases = 0;

  ini::IniFile inputIni;
  inputIni.load(pInputFileValue);
  if(inputIni.count(EnsembleInfoReader::k_EnsembleInfo) == 0)
  {
    return MakePreflightErrorResult(-13001, fmt::format("Could not find the group name {} from the input file '{}'", EnsembleInfoReader::k_EnsembleInfo, pInputFileValue));
  }
  if(inputIni[EnsembleInfoReader::k_EnsembleInfo].count(EnsembleInfoReader::k_NumberPhases) == 0)
  {
    return MakePreflightErrorResult(
        -13002, fmt::format("Could not find the key {} in the {} group from the input file '{}'", EnsembleInfoReader::k_NumberPhases, EnsembleInfoReader::k_EnsembleInfo, pInputFileValue));
  }
  numPhases = inputIni[EnsembleInfoReader::k_EnsembleInfo][EnsembleInfoReader::k_NumberPhases].as<int32>(); // read number of phases from input file

  if(0 == numPhases) // 0 was entered as the Number_Phases
  {
    return MakePreflightErrorResult(-13003, fmt::format("The {} in the {} group must be greater than 0", EnsembleInfoReader::k_NumberPhases, EnsembleInfoReader::k_EnsembleInfo));
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
Result<> EnsembleInfoReaderFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  EnsembleInfoReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key).string();
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellEnsembleAttributeMatrixName = inputValues.DataContainerName.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));
  inputValues.CrystalStructuresArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_CrystalStructuresArrayName_Key));
  inputValues.PhaseTypesArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_PhaseTypesArrayName_Key));

  return EnsembleInfoReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
