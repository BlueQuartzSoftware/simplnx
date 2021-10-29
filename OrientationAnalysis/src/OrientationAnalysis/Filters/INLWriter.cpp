#include "INLWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string INLWriter::name() const
{
  return FilterTraits<INLWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string INLWriter::className() const
{
  return FilterTraits<INLWriter>::className;
}

//------------------------------------------------------------------------------
Uuid INLWriter::uuid() const
{
  return FilterTraits<INLWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string INLWriter::humanName() const
{
  return "Export INL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> INLWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters INLWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaterialNameArrayPath_Key, "Material Names", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer INLWriter::clone() const
{
  return std::make_unique<INLWriter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> INLWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<INLWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> INLWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
