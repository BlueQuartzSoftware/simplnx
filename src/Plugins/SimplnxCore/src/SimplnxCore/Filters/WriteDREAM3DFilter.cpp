#include "WriteDREAM3DFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace
{
constexpr nx::core::int32 k_NoExportPathError = -1;
constexpr nx::core::int32 k_FailedFindPipelineError = -15;
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
  return "Write DREAM3D NX File";
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
IFilter::UniquePointer WriteDREAM3DFilter::clone() const
{
  return std::make_unique<WriteDREAM3DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto exportFilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  if(exportFilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoExportPathError, "Export file path not provided."}})};
  }
  return {};
}

//------------------------------------------------------------------------------
Result<> WriteDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto atomicFileResult = AtomicFile::Create(args.value<FileSystemPathParameter::ValueType>(k_ExportFilePath));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  auto exportFilePath = atomicFile.tempFilePath();
  auto writeXdmf = args.value<bool>(k_WriteXdmf);

  Pipeline pipeline;

  if(pipelineNode != nullptr)
  {
    auto pipelinePtr = pipelineNode->getPrecedingPipeline();
    if(pipelinePtr == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_FailedFindPipelineError, "Failed to retrieve pipeline."}})};
    }

    pipeline = *pipelinePtr;
  }

  auto results = DREAM3D::WriteFile(exportFilePath, dataStructure, pipeline, writeXdmf);
  if(results.valid())
  {
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
    if(writeXdmf)
    {
      fs::path xdmfFilePath = exportFilePath.replace_extension(".xdmf");
      std::error_code errorCode;
      fs::rename(xdmfFilePath, args.value<fs::path>(k_ExportFilePath).replace_extension(".xdmf"), errorCode);
      if(errorCode)
      {
        std::string ss = fmt::format("Failed to rename xdmf file with error: '{}'", errorCode.message());
        return MakeErrorResult(errorCode.value(), ss);
      }
    }
  }
  return results;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_WriteXdmfFileKey = "WriteXdmfFile";
constexpr StringLiteral k_WriteTimeSeriesKey = "WriteTimeSeries";
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
