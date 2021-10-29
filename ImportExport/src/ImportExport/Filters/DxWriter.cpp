#include "DxWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DxWriter::name() const
{
  return FilterTraits<DxWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string DxWriter::className() const
{
  return FilterTraits<DxWriter>::className;
}

//------------------------------------------------------------------------------
Uuid DxWriter::uuid() const
{
  return FilterTraits<DxWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string DxWriter::humanName() const
{
  return "Export Dx File (Feature Ids)";
}

//------------------------------------------------------------------------------
std::vector<std::string> DxWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters DxWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_AddSurfaceLayer_Key, "Add Surface Layer", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DxWriter::clone() const
{
  return std::make_unique<DxWriter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> DxWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pAddSurfaceLayerValue = filterArgs.value<bool>(k_AddSurfaceLayer_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DxWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> DxWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pAddSurfaceLayerValue = filterArgs.value<bool>(k_AddSurfaceLayer_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
