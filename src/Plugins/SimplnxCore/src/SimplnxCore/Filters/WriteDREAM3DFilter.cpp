#include "WriteDREAM3DFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteDREAM3D.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace
{
constexpr nx::core::int32 k_NoExportPathError = -1;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteDREAM3DFilter::name() const
{
  return FilterTraits<WriteDREAM3DFilter>::name;
}

//------------------------------------------------------------------------------
std::string WriteDREAM3DFilter::className() const
{
  return FilterTraits<WriteDREAM3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteDREAM3DFilter::uuid() const
{
  return FilterTraits<WriteDREAM3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteDREAM3DFilter::humanName() const
{
  return "Write DREAM3D-NX File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteDREAM3DFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Binary"};
}

//------------------------------------------------------------------------------
Parameters WriteDREAM3DFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Output File Path", "The file path the DataStructure should be written to as an HDF5 file.", "Untitled.dream3d",
                                                          FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::OutputFile, false));
  params.insert(std::make_unique<BoolParameter>(k_WriteXdmf, "Write Xdmf File", "Whether or not to write the data out an XDMF file", true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteDREAM3DFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteDREAM3DFilter::clone() const
{
  return std::make_unique<WriteDREAM3DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                           const ExecutionContext& executionContext) const
{
  auto exportFilePath = filterArgs.value<std::filesystem::path>(k_ExportFilePath);
  if(exportFilePath.empty())
  {
    return MakePreflightErrorResult(k_NoExportPathError, "Export file path not provided.");
  }
  return {};
}

//------------------------------------------------------------------------------
Result<> WriteDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteDREAM3DInputValues inputValues;
  inputValues.ExportFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_ExportFilePath);
  inputValues.WriteXdmfFile = filterArgs.value<BoolParameter::ValueType>(k_WriteXdmf);
  inputValues.PipelineNode = pipelineNode;

  return WriteDREAM3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_WriteXdmfFileKey = "WriteXdmfFile";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteDREAM3DFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteDREAM3DFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_ExportFilePath));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteXdmfFileKey, k_WriteXdmf));
  // Write time series parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
