#include "WriteDREAM3D.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_NoExportPathError = -1;
constexpr nx::core::int32 k_FailedFindPipelineError = -15;
} // namespace

// -----------------------------------------------------------------------------
WriteDREAM3D::WriteDREAM3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteDREAM3DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteDREAM3D::~WriteDREAM3D() noexcept = default;

/**
 * @brief Extracts the preceding Pipeline from the PipelineFilter.
 * This method returns an empty Pipeline if the PipelineFilter is null.
 * @param pipelineNode The PipelineFilter to extract the Pipeline from.
 * @return Result<Pipeline> The target Pipeline or an error message if the process failed to complete.
 */
Result<Pipeline> ExtractPipeline(const PipelineFilter* pipelineNode)
{
  Pipeline pipeline;
  if(pipelineNode != nullptr)
  {
    auto pipelinePtr = pipelineNode->getPrecedingPipeline();
    if(pipelinePtr == nullptr)
    {
      return MakeErrorResult<Pipeline>(k_FailedFindPipelineError, "Failed to retrieve pipeline.");
    }

    pipeline = *pipelinePtr;
  }
  return {pipeline};
}

/**
 * @brief Writes the DREAM3D file to the temp file, commits changes, and then writes the XDMF file if requested.
 * @param atomicFile Temp file for writing the DREAM3D file
 * @param dataStructure DataStructure to be written to file.
 * @param pipeline Pipeline to write to file.
 * @param writeXdmfFile
 * @return Result<>
 */
Result<> WriteDREAM3DFile(AtomicFile& atomicFile, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmfFile, const nx::core::HDF5::DataStructureWriter::WriteOptions& writeOptions)
{
  auto exportFilePath = atomicFile.tempFilePath();

  auto results = DREAM3D::WriteFile(exportFilePath, dataStructure, pipeline, writeXdmfFile, writeOptions);
  if(results.invalid())
  {
    return results;
  }

  // Commit changes to the temp file. Return an invalid Result if errors occured.
  if(auto commitResult = atomicFile.commit(); commitResult.invalid())
  {
    return commitResult;
  }

  // Write the XDMF file if specified
  if(writeXdmfFile)
  {
    // TODO: Double check this
    fs::path xdmfFilePath = exportFilePath.replace_extension(".xdmf");
    std::error_code errorCode;
    fs::rename(xdmfFilePath, fs::path(exportFilePath).replace_extension(".xdmf"), errorCode);

    // If the XDMF file failed to be renamed, return an invalid Result
    if(errorCode)
    {
      std::string ss = fmt::format("Failed to rename xdmf file with error: '{}'", errorCode.message());
      return MakeErrorResult(errorCode.value(), ss);
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> WriteDREAM3D::operator()()
{
  // Create AtomicFile to write.
  auto atomicFileResult = AtomicFile::Create(m_InputValues->ExportFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  // Extract Preceding Pipeline
  Pipeline pipeline;
  if(auto result = ExtractPipeline(m_InputValues->PipelineNode); result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  else
  {
    pipeline = std::move(result.value());
  }

  // Write DREAM.3D file
  nx::core::HDF5::DataStructureWriter::WriteOptions writeOptions;
  writeOptions.compressionLevel = m_InputValues->UseCompression ? m_InputValues->CompressionLevel : 0;
  return WriteDREAM3DFile(atomicFile, m_DataStructure, pipeline, m_InputValues->WriteXdmfFile, writeOptions);
}
