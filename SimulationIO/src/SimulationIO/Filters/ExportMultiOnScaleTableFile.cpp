#include "ExportMultiOnScaleTableFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExportMultiOnScaleTableFile::name() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ExportMultiOnScaleTableFile::className() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::className;
}

//------------------------------------------------------------------------------
Uuid ExportMultiOnScaleTableFile::uuid() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ExportMultiOnScaleTableFile::humanName() const
{
  return "Create Multi OnScale Table File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExportMultiOnScaleTableFile::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ExportMultiOnScaleTableFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPrefix_Key, "Data Container Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MatrixName_Key, "Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ArrayName_Key, "Array Name", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumKeypoints_Key, "Number of Keypoints", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExportMultiOnScaleTableFile::clone() const
{
  return std::make_unique<ExportMultiOnScaleTableFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExportMultiOnScaleTableFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MatrixName_Key);
  auto pArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ArrayName_Key);
  auto pNumKeypointsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  auto pPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string selectedArrays;

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
  auto action = std::make_unique<ExportMultiOnScaleTableFileAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  // These values should have been updated during the preflightImpl(...) method
  preflightResult.outputValues.push_back({"SelectedArrays", selectedArrays});

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExportMultiOnScaleTableFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MatrixName_Key);
  auto pArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ArrayName_Key);
  auto pNumKeypointsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  auto pPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
