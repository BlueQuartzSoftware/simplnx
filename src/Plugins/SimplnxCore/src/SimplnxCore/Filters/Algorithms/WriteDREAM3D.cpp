#include "WriteDREAM3D.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/PathText.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <iterator>
#include <optional>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr int32 k_FailedFindPipelineError = -15;
constexpr int32 k_FilterCancelledError = -1;
constexpr int32 k_StagedXdmfMoveError = -6065;

/**
 * @brief Moves warnings in front of the warnings of a result.
 * @param result Later result; its validity and errors stay unchanged.
 * @param warnings Warnings collected before result; empty after the call.
 */
void PrependWarnings(Result<>& result, std::vector<Warning>& warnings)
{
  result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
  warnings.clear();
}

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
  // AtomicFile::Create() runs the file-replace hook on each final path. Both hooks run before either final file changes.
  auto atomicFileResult = AtomicFile::Create(m_InputValues->ExportFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  std::vector<Warning> hookWarnings = std::move(atomicFileResult.warnings());
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  std::optional<AtomicFile> xdmfAtomicFile;
  if(m_InputValues->WriteXdmfFile)
  {
    auto xdmfAtomicFileResult = AtomicFile::Create(FinalXdmfFilePath(m_InputValues->ExportFilePath));
    if(xdmfAtomicFileResult.invalid())
    {
      Result<> xdmfResult = ConvertResult(std::move(xdmfAtomicFileResult));
      PrependWarnings(xdmfResult, hookWarnings);
      return xdmfResult;
    }
    std::vector<Warning>& xdmfWarnings = xdmfAtomicFileResult.warnings();
    hookWarnings.insert(hookWarnings.end(), std::make_move_iterator(xdmfWarnings.begin()), std::make_move_iterator(xdmfWarnings.end()));
    xdmfAtomicFile.emplace(std::move(xdmfAtomicFileResult.value()));
  }

  Result<> result = writeAndCommit(atomicFile, xdmfAtomicFile.has_value() ? &xdmfAtomicFile.value() : nullptr);
  PrependWarnings(result, hookWarnings);
  return result;
}

// -----------------------------------------------------------------------------
Result<> WriteDREAM3D::writeAndCommit(AtomicFile& atomicFile, AtomicFile* xdmfAtomicFile) const
{
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

  // Components released by the hooks before a cancel stay released; the final files stay unchanged.
  if(m_ShouldCancel)
  {
    return MakeErrorResult(k_FilterCancelledError, "Filter cancelled");
  }

  // The DREAM3D writer stages the .xdmf file next to the staged .dream3d file. It derives the .xdmf path from the .dream3d
  // path and checks the two destinations against each other.
  const fs::path stagedFilePath = atomicFile.tempFilePath();
  const bool writesXdmf = xdmfAtomicFile != nullptr;
  nx::core::HDF5::DataStructureWriter::WriteOptions writeOptions;
  writeOptions.compressionLevel = m_InputValues->UseCompression ? m_InputValues->CompressionLevel : 0;
  Result<> results = DREAM3D::WriteFile(stagedFilePath, m_DataStructure, pipeline, writesXdmf, writeOptions);
  if(results.invalid())
  {
    return results;
  }
  if(m_ShouldCancel)
  {
    Result<> cancelResult = MakeErrorResult(k_FilterCancelledError, "Filter cancelled");
    cancelResult.warnings() = std::move(results.warnings());
    return cancelResult;
  }

  if(writesXdmf)
  {
    // The staged .xdmf file moves into the temporary path of its own AtomicFile, which commits it onto the final .xdmf path.
    const fs::path stagedXdmfFilePath = fs::path(stagedFilePath).replace_extension(".xdmf");
    const fs::path xdmfTempFilePath = xdmfAtomicFile->tempFilePath();
    std::error_code moveError;
    fs::rename(stagedXdmfFilePath, xdmfTempFilePath, moveError);
    if(moveError)
    {
      Result<> moveResult = MakeErrorResult(k_StagedXdmfMoveError,
                                            fmt::format("Cannot move the staged XDMF file '{}' to '{}' before it replaces '{}': {}. No output file was changed.", PathToUtf8Text(stagedXdmfFilePath),
                                                        PathToUtf8Text(xdmfTempFilePath), PathToUtf8Text(FinalXdmfFilePath(m_InputValues->ExportFilePath)), moveError.message()));
      moveResult.warnings() = std::move(results.warnings());
      return moveResult;
    }
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    PrependWarnings(commitResult, results.warnings());
    return commitResult;
  }
  if(writesXdmf)
  {
    // The .dream3d file is already replaced; a failure here leaves the previous .xdmf file in place.
    Result<> xdmfCommitResult = xdmfAtomicFile->commit();
    if(xdmfCommitResult.invalid())
    {
      PrependWarnings(xdmfCommitResult, results.warnings());
      return xdmfCommitResult;
    }
  }
  return results;
}
