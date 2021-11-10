#include "ExportCLIFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExportCLIFile::name() const
{
  return FilterTraits<ExportCLIFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ExportCLIFile::className() const
{
  return FilterTraits<ExportCLIFile>::className;
}

//------------------------------------------------------------------------------
Uuid ExportCLIFile::uuid() const
{
  return FilterTraits<ExportCLIFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ExportCLIFile::humanName() const
{
  return "Export CLI File(s)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExportCLIFile::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters ExportCLIFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_UnitsScaleFactor_Key, "Units Scale Factor", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_Precision_Key, "Precision (places after decimal)", "", 1234356));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputDirectory_Key, "Output File Directory", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputFilePrefix_Key, "Output File Prefix", "", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SplitByGroup_Key, "Split CLI Files by Group", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_EdgeGeometry_Key, "Edge Geometry", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_LayerIdsArrayPath_Key, "Layer Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GroupIdsArrayPath_Key, "Group Ids", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SplitByGroup_Key, k_GroupIdsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExportCLIFile::clone() const
{
  return std::make_unique<ExportCLIFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExportCLIFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUnitsScaleFactorValue = filterArgs.value<float64>(k_UnitsScaleFactor_Key);
  auto pPrecisionValue = filterArgs.value<int32>(k_Precision_Key);
  auto pOutputDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pSplitByGroupValue = filterArgs.value<bool>(k_SplitByGroup_Key);
  auto pEdgeGeometryValue = filterArgs.value<DataPath>(k_EdgeGeometry_Key);
  auto pLayerIdsArrayPathValue = filterArgs.value<DataPath>(k_LayerIdsArrayPath_Key);
  auto pGroupIdsArrayPathValue = filterArgs.value<DataPath>(k_GroupIdsArrayPath_Key);

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
  auto action = std::make_unique<ExportCLIFileAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExportCLIFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUnitsScaleFactorValue = filterArgs.value<float64>(k_UnitsScaleFactor_Key);
  auto pPrecisionValue = filterArgs.value<int32>(k_Precision_Key);
  auto pOutputDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pSplitByGroupValue = filterArgs.value<bool>(k_SplitByGroup_Key);
  auto pEdgeGeometryValue = filterArgs.value<DataPath>(k_EdgeGeometry_Key);
  auto pLayerIdsArrayPathValue = filterArgs.value<DataPath>(k_LayerIdsArrayPath_Key);
  auto pGroupIdsArrayPathValue = filterArgs.value<DataPath>(k_GroupIdsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
