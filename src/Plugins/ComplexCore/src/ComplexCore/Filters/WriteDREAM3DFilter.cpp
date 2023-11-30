#include "WriteDREAM3DFilter.hpp"

#include "complex/Common/AtomicFile.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/SIMPLConversion.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace
{
constexpr complex::int32 k_NoExportPathError = -1;
constexpr complex::int32 k_FailedFindPipelineError = -15;
} // namespace

namespace complex
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Export File Path", "The file path the DataStructure should be written to as an HDF5 file.", "",
                                                          FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteXdmf, "Write Xdmf File", "Whether or not to write the data out an xdmf file", true));
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
  AtomicFile atomicFile(args.value<std::filesystem::path>(k_ExportFilePath), false);

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
    atomicFile.commit();
    if(writeXdmf)
    {
      fs::path xdmfFilePath = exportFilePath.replace_extension(".xdmf");
      fs::rename(xdmfFilePath, args.value<fs::path>(k_ExportFilePath).replace_extension(".xdmf"));
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
} // namespace complex
