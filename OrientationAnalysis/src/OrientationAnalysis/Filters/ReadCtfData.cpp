#include "ReadCtfData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadCtfData::name() const
{
  return FilterTraits<ReadCtfData>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadCtfData::className() const
{
  return FilterTraits<ReadCtfData>::className;
}

//------------------------------------------------------------------------------
Uuid ReadCtfData::uuid() const
{
  return FilterTraits<ReadCtfData>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadCtfData::humanName() const
{
  return "Import Oxford Instr. EBSD Data (.ctf)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadCtfData::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ReadCtfData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_DegreesToRadians_Key, "Convert Eulers to Radians", "", false));
  params.insert(std::make_unique<BoolParameter>(k_EdaxHexagonalAlignment_Key, "Convert Hexagonal X-Axis to Edax Standard", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadCtfData::clone() const
{
  return std::make_unique<ReadCtfData>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ReadCtfData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDegreesToRadiansValue = filterArgs.value<bool>(k_DegreesToRadians_Key);
  auto pEdaxHexagonalAlignmentValue = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReadCtfDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ReadCtfData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDegreesToRadiansValue = filterArgs.value<bool>(k_DegreesToRadians_Key);
  auto pEdaxHexagonalAlignmentValue = filterArgs.value<bool>(k_EdaxHexagonalAlignment_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
