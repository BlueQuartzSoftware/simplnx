#include "FeatureDataCSVWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::name() const
{
  return FilterTraits<FeatureDataCSVWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::className() const
{
  return FilterTraits<FeatureDataCSVWriter>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureDataCSVWriter::uuid() const
{
  return FilterTraits<FeatureDataCSVWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriter::humanName() const
{
  return "Export Feature Data as CSV File";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureDataCSVWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters FeatureDataCSVWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_FeatureDataFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteNeighborListData_Key, "Write Neighbor Data", "", false));
  params.insert(std::make_unique<BoolParameter>(k_WriteNumFeaturesLine_Key, "Write Number of Features Line", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceInt_Key, "Delimiter", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureDataCSVWriter::clone() const
{
  return std::make_unique<FeatureDataCSVWriter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FeatureDataCSVWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureDataFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FeatureDataCSVWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FeatureDataCSVWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureDataFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
