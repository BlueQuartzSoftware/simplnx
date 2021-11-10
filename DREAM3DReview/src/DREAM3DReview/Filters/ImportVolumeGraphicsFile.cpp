#include "ImportVolumeGraphicsFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFile::name() const
{
  return FilterTraits<ImportVolumeGraphicsFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFile::className() const
{
  return FilterTraits<ImportVolumeGraphicsFile>::className;
}

//------------------------------------------------------------------------------
Uuid ImportVolumeGraphicsFile::uuid() const
{
  return FilterTraits<ImportVolumeGraphicsFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFile::humanName() const
{
  return "Import Volume Graphics File (.vgi/.vol)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportVolumeGraphicsFile::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportVolumeGraphicsFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_VGHeaderFile_Key, "VolumeGraphics .vgi File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_DataContainerName_Key, "Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DensityArrayName_Key, "Density", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportVolumeGraphicsFile::clone() const
{
  return std::make_unique<ImportVolumeGraphicsFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportVolumeGraphicsFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pVGHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VGHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);

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
  auto action = std::make_unique<ImportVolumeGraphicsFileAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ImportVolumeGraphicsFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pVGHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VGHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
