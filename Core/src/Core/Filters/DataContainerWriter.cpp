#include "DataContainerWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DataContainerWriter::name() const
{
  return FilterTraits<DataContainerWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string DataContainerWriter::className() const
{
  return FilterTraits<DataContainerWriter>::className;
}

//------------------------------------------------------------------------------
Uuid DataContainerWriter::uuid() const
{
  return FilterTraits<DataContainerWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string DataContainerWriter::humanName() const
{
  return "Write DREAM.3D Data File";
}

//------------------------------------------------------------------------------
std::vector<std::string> DataContainerWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters DataContainerWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteXdmfFile_Key, "Write Xdmf File", "", false));
  params.insert(std::make_unique<BoolParameter>(k_WriteTimeSeries_Key, "Include Xdmf Time Markers", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DataContainerWriter::clone() const
{
  return std::make_unique<DataContainerWriter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DataContainerWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteXdmfFileValue = filterArgs.value<bool>(k_WriteXdmfFile_Key);
  auto pWriteTimeSeriesValue = filterArgs.value<bool>(k_WriteTimeSeries_Key);

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
  auto action = std::make_unique<DataContainerWriterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> DataContainerWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteXdmfFileValue = filterArgs.value<bool>(k_WriteXdmfFile_Key);
  auto pWriteTimeSeriesValue = filterArgs.value<bool>(k_WriteTimeSeries_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
