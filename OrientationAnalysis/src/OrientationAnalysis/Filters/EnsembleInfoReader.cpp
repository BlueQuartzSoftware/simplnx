#include "EnsembleInfoReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EnsembleInfoReader::name() const
{
  return FilterTraits<EnsembleInfoReader>::name.str();
}

//------------------------------------------------------------------------------
std::string EnsembleInfoReader::className() const
{
  return FilterTraits<EnsembleInfoReader>::className;
}

//------------------------------------------------------------------------------
Uuid EnsembleInfoReader::uuid() const
{
  return FilterTraits<EnsembleInfoReader>::uuid;
}

//------------------------------------------------------------------------------
std::string EnsembleInfoReader::humanName() const
{
  return "Import Ensemble Info File";
}

//------------------------------------------------------------------------------
std::vector<std::string> EnsembleInfoReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters EnsembleInfoReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input Ensemble Info File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EnsembleInfoReader::clone() const
{
  return std::make_unique<EnsembleInfoReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EnsembleInfoReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);

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
  auto action = std::make_unique<EnsembleInfoReaderAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> EnsembleInfoReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
