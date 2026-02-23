#include "ExtractPipelineToFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExtractPipelineToFile.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

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
  return "Extract DREAM3D-NX Pipeline To File";
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ImportFileData, "Input DREAM3D File Path", "The file path to the .dream3d that holds the pipeline to be extracted.",
                                                          FileSystemPathParameter::ValueType{}, FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile, "Output File Path", "The file path in which to save the extracted pipeline", FileSystemPathParameter::ValueType{},
                                                          FileSystemPathParameter::ExtensionsType{Pipeline::k_Extension, Pipeline::k_SIMPLExtension}, FileSystemPathParameter::PathType::OutputFile));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ExtractPipelineToFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractPipelineToFileFilter::clone() const
{
  return std::make_unique<ExtractPipelineToFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractPipelineToFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto importFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_ImportFileData);
  auto outputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile);

  Result<nlohmann::json> pipelineResult = DREAM3D::ImportPipelineJsonFromFile(importFile);
  if(pipelineResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions, nlohmann::json>(std::move(pipelineResult))};
  }

  Result<OutputActions> results;

  const nlohmann::json pipelineJson = pipelineResult.value();
  if(pipelineJson.empty())
  {
    return {MakePreflightErrorResult(-2570, fmt::format("No DREAM3D pipeline exists inside dream3d file '{}'.", importFile.string()))};
  }

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
Result<> ExtractPipelineToFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ExtractPipelineToFileInputValues inputValues;
  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_ImportFileData);
  inputValues.OutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile);

  return ExtractPipelineToFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

Result<Arguments> ExtractPipelineToFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  return {};
}
} // namespace nx::core
