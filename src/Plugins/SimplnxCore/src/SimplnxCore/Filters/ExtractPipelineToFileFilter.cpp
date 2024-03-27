#include "ExtractPipelineToFileFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <nlohmann/json.hpp>

namespace
{
constexpr nx::core::StringLiteral k_ImportedPipeline = "Imported Pipeline";
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExtractPipelineToFileFilter::name() const
{
  return FilterTraits<ExtractPipelineToFileFilter>::name;
}

//------------------------------------------------------------------------------
std::string ExtractPipelineToFileFilter::className() const
{
  return FilterTraits<ExtractPipelineToFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractPipelineToFileFilter::uuid() const
{
  return FilterTraits<ExtractPipelineToFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractPipelineToFileFilter::humanName() const
{
  return "Extract DREAM.3D Pipeline To File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractPipelineToFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Output", "Write", "Export", "Pipeline", "JSON"};
}

//------------------------------------------------------------------------------
Parameters ExtractPipelineToFileFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ImportFileData, "Input DREAM3D File Path", "The file path to the .dream3d that holds the pipeline to be extracted.",
                                                          FileSystemPathParameter::ValueType{}, FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile, "Output File Path", "The file path in which to save the extracted pipeline", FileSystemPathParameter::ValueType{},
                                                          FileSystemPathParameter::ExtensionsType{Pipeline::k_Extension, Pipeline::k_SIMPLExtension}, FileSystemPathParameter::PathType::OutputFile));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractPipelineToFileFilter::clone() const
{
  return std::make_unique<ExtractPipelineToFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractPipelineToFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  const auto importFile = args.value<FileSystemPathParameter::ValueType>(k_ImportFileData);
  auto outputFile = args.value<FileSystemPathParameter::ValueType>(k_OutputFile);

  Result<nlohmann::json> pipelineResult = DREAM3D::ImportPipelineJsonFromFile(importFile);
  if(pipelineResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions, nlohmann::json>(std::move(pipelineResult))};
  }

  Result<OutputActions> results;

  const nlohmann::json pipelineJson = pipelineResult.value();
  const bool isLegacy = pipelineJson.contains(nx::core::Pipeline::k_SIMPLPipelineBuilderKey);

  fs::path finalOutputPath = outputFile;
  std::string extension = isLegacy ? Pipeline::k_SIMPLExtension : Pipeline::k_Extension;
  if(!finalOutputPath.has_extension())
  {
    finalOutputPath.concat(extension);
    results.warnings().push_back(Warning{
        -2580, fmt::format("Output file '{}' is missing an extension. A {} extension will be added to the provided output file so that the extracted pipeline will be written to the file at path '{}'",
                           outputFile.string(), extension, finalOutputPath.string())});
  }
  if(finalOutputPath.extension().string() != extension)
  {
    finalOutputPath.replace_extension(extension);
    results.warnings().push_back(
        Warning{-2581, fmt::format("Output file '{}' has the incorrect extension. A {} extension will be used instead so that the extracted pipeline will be written to the file at path '{}'",
                                   outputFile.string(), extension, finalOutputPath.string())});
  }

  return {results};
}

//------------------------------------------------------------------------------
Result<> ExtractPipelineToFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  const auto importFile = args.value<FileSystemPathParameter::ValueType>(k_ImportFileData);
  auto outputFile = args.value<FileSystemPathParameter::ValueType>(k_OutputFile);

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
  AtomicFile atomicFile(outputFile.string(), false);
  {
    const fs::path exportFilePath = atomicFile.tempFilePath();
    std::ofstream fOut(exportFilePath.string(), std::ofstream::out); // test name resolution and create file
    if(!fOut.is_open())
    {
      return MakeErrorResult(-2582, fmt::format("Error opening output path {}", exportFilePath.string()));
    }

    fOut << pipelineJson.dump(2);
  }
  atomicFile.commit();
  return {};
}

Result<Arguments> ExtractPipelineToFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  return {};
}
} // namespace nx::core
