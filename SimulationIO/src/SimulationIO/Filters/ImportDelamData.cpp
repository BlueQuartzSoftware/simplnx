#include "ImportDelamData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportDelamData::name() const
{
  return FilterTraits<ImportDelamData>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportDelamData::className() const
{
  return FilterTraits<ImportDelamData>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDelamData::uuid() const
{
  return FilterTraits<ImportDelamData>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDelamData::humanName() const
{
  return "Import Delam Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDelamData::defaultTags() const
{
  return {"#IO", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ImportDelamData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_CSDGMFile_Key, "CSDGM File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_BvidStdOutFile_Key, "Bvid StdOut File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<Float32Parameter>(k_InterfaceThickness_Key, "Interface Thickness", "", 1.23345f));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "Data Container", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_DataArrayName_Key, "Data Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportDelamData::clone() const
{
  return std::make_unique<ImportDelamData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportDelamData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCSDGMFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CSDGMFile_Key);
  auto pBvidStdOutFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BvidStdOutFile_Key);
  auto pInterfaceThicknessValue = filterArgs.value<float32>(k_InterfaceThickness_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayName_Key);

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
  auto action = std::make_unique<ImportDelamDataAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ImportDelamData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCSDGMFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CSDGMFile_Key);
  auto pBvidStdOutFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BvidStdOutFile_Key);
  auto pInterfaceThicknessValue = filterArgs.value<float32>(k_InterfaceThickness_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
