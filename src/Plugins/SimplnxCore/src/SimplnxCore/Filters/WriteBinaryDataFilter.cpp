#include "WriteBinaryDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteBinaryData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::name() const
{
  return FilterTraits<WriteBinaryDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::className() const
{
  return FilterTraits<WriteBinaryDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteBinaryDataFilter::uuid() const
{
  return FilterTraits<WriteBinaryDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::humanName() const
{
  return "Write Binary Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteBinaryDataFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Binary"};
}

//------------------------------------------------------------------------------
Parameters WriteBinaryDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Endianess_Key, "Endianess", "Default is little endian", to_underlying(Endianess::Little),
                                                   ChoicesParameter::Choices{"Little Endian", "Big Endian"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "The file extension for the output file", ".bin"));
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "The arrays to be exported to a binary file",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteBinaryDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteBinaryDataFilter::clone() const
{
  return std::make_unique<WriteBinaryDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteBinaryDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  return {};
}

//------------------------------------------------------------------------------
Result<> WriteBinaryDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteBinaryDataInputValues inputValues;
  inputValues.EndianIndex = filterArgs.value<ChoicesParameter::ValueType>(k_Endianess_Key);
  inputValues.InputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  return WriteBinaryData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
