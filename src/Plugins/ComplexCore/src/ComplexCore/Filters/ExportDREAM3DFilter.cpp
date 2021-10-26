#include "ExportDREAM3DFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

namespace
{
constexpr complex::int32 k_NoExportPathError = -1;
constexpr complex::int32 k_NoParentPathError = -2;
constexpr complex::int32 k_FailedFileWriterError = -14;
constexpr complex::int32 k_FailedFindPipelineError = -15;
} // namespace

namespace complex
{
std::string ExportDREAM3DFilter::name() const
{
  return FilterTraits<ExportDREAM3DFilter>::name;
}

std::string ExportDREAM3DFilter::className() const
{
  return FilterTraits<ExportDREAM3DFilter>::className;
}

Uuid ExportDREAM3DFilter::uuid() const
{
  return FilterTraits<ExportDREAM3DFilter>::uuid;
}

std::string ExportDREAM3DFilter::humanName() const
{
  return "Write DREAM.3D File";
}

Parameters ExportDREAM3DFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Export File Path", "The file path the DataStructure should be written to as an HDF5 file.", "",
                                                          FileSystemPathParameter::PathType::OutputFile));
  return params;
}

IFilter::UniquePointer ExportDREAM3DFilter::clone() const
{
  return std::make_unique<ExportDREAM3DFilter>();
}

Result<OutputActions> ExportDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto exportFilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  if(exportFilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoExportPathError, "Export file path not provided."}})};
  }
  auto exportDirectoryPath = exportFilePath.parent_path();
  if(std::filesystem::exists(exportDirectoryPath) == false)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoParentPathError, "Export parent directory does not exist."}})};
  }
  return {};
}

Result<> ExportDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto exportFilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  H5::FileWriter fileWriter(exportFilePath);
  if(!fileWriter.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_FailedFileWriterError, "Failed to initialize H5:FileWriter."}})};
  }

  auto pipelinePtr = pipelineNode->getPrecedingPipeline();
  if(pipelinePtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_FailedFindPipelineError, "Failed to retrieve pipeline."}})};
  }
  Pipeline pipeline = *pipelinePtr.get();

  auto errorCode = DREAM3D::WriteFile(fileWriter, {pipeline, dataStructure});
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to write .dream3d file."}})};
  }

  return {};
}
} // namespace complex
