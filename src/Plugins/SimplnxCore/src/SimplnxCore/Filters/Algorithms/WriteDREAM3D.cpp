#include "WriteDREAM3D.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <memory>
#include <system_error>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr int32 k_FailedFindPipelineError = -15;
constexpr int32 k_FilterCancelledError = -1;

/**
 * @brief Returns the final .xdmf path that the filter renames the temporary .xdmf file to.
 * @param exportFilePath Final DREAM3D file path.
 * @return The path next to exportFilePath with its stem and the .xdmf extension.
 */
fs::path FinalXdmfFilePath(const fs::path& exportFilePath)
{
  return exportFilePath.parent_path() / exportFilePath.stem().concat(".xdmf");
}
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

// -----------------------------------------------------------------------------
Result<> WriteDREAM3D::operator()()
{
  auto atomicFileResult = AtomicFile::Create(m_InputValues->ExportFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  Pipeline pipeline;

  if(m_InputValues->PipelineNode != nullptr)
  {
    auto pipelinePtr = m_InputValues->PipelineNode->getPrecedingPipeline();
    if(pipelinePtr == nullptr)
    {
      return MakeErrorResult(k_FailedFindPipelineError, "Failed to retrieve pipeline.");
    }

    pipeline = *pipelinePtr;
  }

  if(m_ShouldCancel)
  {
    return MakeErrorResult(k_FilterCancelledError, "Filter cancelled");
  }

  // The writer below targets the temporary file, so its own hook never sees the final paths. A store that reads the final file or the
  // final .xdmf file must be released here, before either one is replaced.
  const fs::path finalXdmfFilePath = FinalXdmfFilePath(m_InputValues->ExportFilePath);
  const std::shared_ptr<Application> application = Application::GetOrCreateInstance();
  DataIOCollection& ioCollection = application->getIOCollection();
  Result<> hooksResult = ioCollection.beforeFileWrite(m_DataStructure, m_InputValues->ExportFilePath);
  if(hooksResult.valid() && m_InputValues->WriteXdmfFile)
  {
    Result<> sidecarHookResult = ioCollection.beforeFileWrite(m_DataStructure, finalXdmfFilePath);
    sidecarHookResult.warnings().insert(sidecarHookResult.warnings().begin(), hooksResult.warnings().begin(), hooksResult.warnings().end());
    hooksResult = std::move(sidecarHookResult);
  }
  if(hooksResult.invalid())
  {
    return hooksResult;
  }

  // Stores released before a cancel stay released; the final files stay unchanged.
  Result<> result = m_ShouldCancel ? MakeErrorResult(k_FilterCancelledError, "Filter cancelled") : writeAndCommit(atomicFile, pipeline, finalXdmfFilePath);
  result.warnings().insert(result.warnings().begin(), hooksResult.warnings().begin(), hooksResult.warnings().end());
  return result;
}

// -----------------------------------------------------------------------------
Result<> WriteDREAM3D::writeAndCommit(AtomicFile& atomicFile, const Pipeline& pipeline, const fs::path& finalXdmfFilePath) const
{
  auto exportFilePath = atomicFile.tempFilePath();
  auto writeXdmf = m_InputValues->WriteXdmfFile;

  nx::core::HDF5::DataStructureWriter::WriteOptions writeOptions;
  writeOptions.compressionLevel = m_InputValues->UseCompression ? m_InputValues->CompressionLevel : 0;
  auto results = DREAM3D::WriteFile(exportFilePath, m_DataStructure, pipeline, writeXdmf, writeOptions);
  if(results.valid())
  {
    if(m_ShouldCancel)
    {
      Result<> cancelResult = MakeErrorResult(k_FilterCancelledError, "Filter cancelled");
      cancelResult.warnings() = std::move(results.warnings());
      return cancelResult;
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      commitResult.warnings().insert(commitResult.warnings().begin(), results.warnings().begin(), results.warnings().end());
      return commitResult;
    }
    if(writeXdmf)
    {
      fs::path xdmfFilePath = exportFilePath.replace_extension(".xdmf");
      std::error_code errorCode;
      fs::rename(xdmfFilePath, finalXdmfFilePath, errorCode);
      if(errorCode)
      {
        std::string ss = fmt::format("Failed to rename xdmf file with error: '{}'", errorCode.message());
        Result<> renameResult = MakeErrorResult(errorCode.value(), ss);
        renameResult.warnings() = std::move(results.warnings());
        return renameResult;
      }
    }
  }
  return results;
}
