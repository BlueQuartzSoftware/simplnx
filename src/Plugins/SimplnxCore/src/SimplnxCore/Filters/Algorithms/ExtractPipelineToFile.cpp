#include "ExtractPipelineToFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
/**
 * @brief Writes the pipeline JSON to the temporary file and commits it onto the output path.
 * @param atomicFile Temporary file whose commit() replaces the output file.
 * @param pipelineJson Pipeline to write.
 * @param shouldCancel Stops before the commit when true.
 * @return Success, or the open, cancel or commit error.
 */
Result<> WriteAndCommit(AtomicFile& atomicFile, const nlohmann::json& pipelineJson, const std::atomic_bool& shouldCancel)
{
  {
    const fs::path exportFilePath = atomicFile.tempFilePath();
    std::ofstream fOut(exportFilePath.string(), std::ofstream::out); // test name resolution and create file
    if(!fOut.is_open())
    {
      return MakeErrorResult(-2582, fmt::format("Error opening output path {}", exportFilePath.string()));
    }

    fOut << pipelineJson.dump(2);
  }
  if(shouldCancel)
  {
    return MakeErrorResult(-1, "Filter cancelled");
  }
  return atomicFile.commit();
}
} // namespace

// -----------------------------------------------------------------------------
ExtractPipelineToFile::ExtractPipelineToFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ExtractPipelineToFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractPipelineToFile::~ExtractPipelineToFile() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ExtractPipelineToFile::operator()()
{
  const auto importFile = m_InputValues->InputFilePath;
  auto outputFile = m_InputValues->OutputFilePath;

  Result<nlohmann::json> pipelineResult = DREAM3D::ImportPipelineJsonFromFile(importFile);
  if(pipelineResult.invalid())
  {
    return ConvertResult<nlohmann::json>(std::move(pipelineResult));
  }
  const nlohmann::json pipelineJson = pipelineResult.value();
  const bool isLegacy = pipelineJson.contains(nx::core::Pipeline::k_SIMPLPipelineBuilderKey);

  std::string extension = isLegacy ? Pipeline::k_SIMPLExtension : Pipeline::k_Extension;
  if(!outputFile.has_extension())
  {
    outputFile.concat(extension);
  }
  if(outputFile.extension().string() != extension)
  {
    outputFile.replace_extension(extension);
  }
  auto atomicFileResult = AtomicFile::Create(outputFile);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  Result<> result = WriteAndCommit(atomicFile, pipelineJson, m_ShouldCancel);
  // The file-replace hook in AtomicFile::Create() can report warnings; they come first.
  result.warnings().insert(result.warnings().begin(), atomicFileResult.warnings().begin(), atomicFileResult.warnings().end());
  return result;
}
