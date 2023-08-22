#include "ExportDREAM3DFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

namespace
{
constexpr complex::int32 k_NoExportPathError = -1;
constexpr complex::int32 k_FailedFindPipelineError = -15;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ExportDREAM3DFilter::name() const
{
  return FilterTraits<ExportDREAM3DFilter>::name;
}

//------------------------------------------------------------------------------
std::string ExportDREAM3DFilter::className() const
{
  return FilterTraits<ExportDREAM3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExportDREAM3DFilter::uuid() const
{
  return FilterTraits<ExportDREAM3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExportDREAM3DFilter::humanName() const
{
  return "Write DREAM3D NX File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExportDREAM3DFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Binary"};
}

//------------------------------------------------------------------------------
Parameters ExportDREAM3DFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Export File Path", "The file path the DataStructure should be written to as an HDF5 file.", "",
                                                          FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteXdmf, "Write Xdmf File", "Whether or not to write the data out an xdmf file", true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExportDREAM3DFilter::clone() const
{
  return std::make_unique<ExportDREAM3DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExportDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto exportFilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  if(exportFilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoExportPathError, "Export file path not provided."}})};
  }
  return {};
}

//------------------------------------------------------------------------------
Result<> ExportDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto exportFilePath = args.value<std::filesystem::path>(k_ExportFilePath);
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
  return results;
}
} // namespace complex
