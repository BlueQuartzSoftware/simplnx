#include "WriteDREAM3D.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr int32 k_FailedFindPipelineError = -15;
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
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Writing DREAM3D file...");

  auto atomicFileResult = AtomicFile::Create(m_InputValues->ExportFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  auto exportFilePath = atomicFile.tempFilePath();
  auto writeXdmf = m_InputValues->WriteXdmfFile;

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

  auto results = DREAM3D::WriteFile(exportFilePath, m_DataStructure, pipeline, writeXdmf);
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
      fs::rename(xdmfFilePath, m_InputValues->ExportFilePath.parent_path() / m_InputValues->ExportFilePath.stem().concat(".xdmf"), errorCode);
      if(errorCode)
      {
        std::string ss = fmt::format("Failed to rename xdmf file with error: '{}'", errorCode.message());
        return MakeErrorResult(errorCode.value(), ss);
      }
    }
  }
  return results;
}
