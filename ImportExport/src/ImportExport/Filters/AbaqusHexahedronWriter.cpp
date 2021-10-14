#include "AbaqusHexahedronWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string AbaqusHexahedronWriter::name() const
{
  return FilterTraits<AbaqusHexahedronWriter>::name.str();
}

Uuid AbaqusHexahedronWriter::uuid() const
{
  return FilterTraits<AbaqusHexahedronWriter>::uuid;
}

std::string AbaqusHexahedronWriter::humanName() const
{
  return "Abaqus Hexahedron Exporter";
}

Parameters AbaqusHexahedronWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_HourglassStiffness_Key, "Hourglass Stiffness", "", 1234356));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "", "SomeString"));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));

  return params;
}

IFilter::UniquePointer AbaqusHexahedronWriter::clone() const
{
  return std::make_unique<AbaqusHexahedronWriter>();
}

Result<OutputActions> AbaqusHexahedronWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pHourglassStiffnessValue = filterArgs.value<int32>(k_HourglassStiffness_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AbaqusHexahedronWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> AbaqusHexahedronWriter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pHourglassStiffnessValue = filterArgs.value<int32>(k_HourglassStiffness_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
