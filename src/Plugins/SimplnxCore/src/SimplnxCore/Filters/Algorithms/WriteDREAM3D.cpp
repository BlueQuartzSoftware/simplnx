#include "WriteDREAM3D.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/FileReplacement.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <utility>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
constexpr int32 k_FailedFindPipelineError = -15;
constexpr int32 k_XdmfReplacementError = -78500;
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

  const auto exportFilePath = atomicFile.tempFilePath();
  const bool writeXdmf = m_InputValues->WriteXdmfFile;

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

  nx::core::HDF5::DataStructureWriter::WriteOptions writeOptions;
  writeOptions.compressionLevel = m_InputValues->UseCompression ? m_InputValues->CompressionLevel : 0;
  auto results = DREAM3D::WriteFile(exportFilePath, m_DataStructure, pipeline, writeXdmf, writeOptions);
  if(results.invalid())
  {
    return results;
  }
  if(m_ShouldCancel)
  {
    return MakeErrorResult(-1, "Filter cancelled");
  }
  auto commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }

  if(writeXdmf)
  {
    fs::path temporarySidecar = exportFilePath;
    temporarySidecar.replace_extension(".xdmf");
    fs::path destinationSidecar = m_InputValues->ExportFilePath;
    destinationSidecar.replace_extension(".xdmf");
    const auto outcome = InstallReplacementFile(temporarySidecar, destinationSidecar);
    if(outcome.Error)
    {
      return MakeErrorResult(k_XdmfReplacementError, DescribeFileReplacementFailure(outcome, destinationSidecar));
    }
  }
  return results;
}
