#include "DataContainerWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string DataContainerWriter::name() const
{
  return FilterTraits<DataContainerWriter>::name.str();
}

std::string DataContainerWriter::className() const
{
  return FilterTraits<DataContainerWriter>::className;
}

Uuid DataContainerWriter::uuid() const
{
  return FilterTraits<DataContainerWriter>::uuid;
}

std::string DataContainerWriter::humanName() const
{
  return "Write DREAM.3D Data File";
}

Parameters DataContainerWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteXdmfFile_Key, "Write Xdmf File", "", false));
  params.insert(std::make_unique<BoolParameter>(k_WriteTimeSeries_Key, "Include Xdmf Time Markers", "", false));

  return params;
}

IFilter::UniquePointer DataContainerWriter::clone() const
{
  return std::make_unique<DataContainerWriter>();
}

Result<OutputActions> DataContainerWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteXdmfFileValue = filterArgs.value<bool>(k_WriteXdmfFile_Key);
  auto pWriteTimeSeriesValue = filterArgs.value<bool>(k_WriteTimeSeries_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DataContainerWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> DataContainerWriter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
