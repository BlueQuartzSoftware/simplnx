#include "AvizoUniformCoordinateWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriter::name() const
{
  return FilterTraits<AvizoUniformCoordinateWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriter::className() const
{
  return FilterTraits<AvizoUniformCoordinateWriter>::className;
}

//------------------------------------------------------------------------------
Uuid AvizoUniformCoordinateWriter::uuid() const
{
  return FilterTraits<AvizoUniformCoordinateWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriter::humanName() const
{
  return "Avizo Uniform Coordinate Exporter";
}

//------------------------------------------------------------------------------
std::vector<std::string> AvizoUniformCoordinateWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters AvizoUniformCoordinateWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "FeatureIds", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AvizoUniformCoordinateWriter::clone() const
{
  return std::make_unique<AvizoUniformCoordinateWriter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AvizoUniformCoordinateWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteBinaryFileValue = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pUnitsValue = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AvizoUniformCoordinateWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AvizoUniformCoordinateWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteBinaryFileValue = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pUnitsValue = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
