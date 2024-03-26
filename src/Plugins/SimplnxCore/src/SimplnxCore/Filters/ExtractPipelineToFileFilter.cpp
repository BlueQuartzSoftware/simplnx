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
  return {className(), "IO", "Input", "Read", "Import", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters ExtractPipelineToFileFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_ImportFileData, "Import File Path", "The DREAM.3D file path the pipeline should be taken from", FileSystemPathParameter::ValueType{},
                                                          FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputDir, "Output Directory", "The directory in which to save the extracted pipeline file", FileSystemPathParameter::ValueType{},
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputFileName, "Output File Name", "The directory in which to save the extracted pipeline file", ""));
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
  if(args.value<StringParameter::ValueType>(k_OutputFileName).empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-2580, "Output file name cannot be empty."}})};
  }
  OutputActions actions;
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExtractPipelineToFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  const auto importFile = args.value<FileSystemPathParameter::ValueType>(k_ImportFileData);
  auto outputDir = args.value<FileSystemPathParameter::ValueType>(k_OutputDir);
  auto outputFileName = args.value<StringParameter::ValueType>(k_OutputFileName);

  Result<nlohmann::json> pipelineResult = DREAM3D::ImportPipelineJsonFromFile(importFile);
  if(pipelineResult.invalid())
  {
    return ConvertResult<nlohmann::json>(std::move(pipelineResult));
  }
  const nlohmann::json pipelineJson = pipelineResult.value();
  const bool isLegacy = pipelineJson.contains(nx::core::Pipeline::k_SIMPLPipelineBuilderKey);

  std::string extension = isLegacy ? Pipeline::k_SIMPLExtension : Pipeline::k_Extension;
  std::string outputPath = outputDir.string() + "/" + outputFileName + extension;
  AtomicFile atomicFile(outputPath, false);
  {
    const fs::path exportFilePath = atomicFile.tempFilePath();
    std::ofstream fOut(exportFilePath.string(), std::ofstream::out); // test name resolution and create file
    if(!fOut.is_open())
    {
      return MakeErrorResult(-2581, fmt::format("Error opening output path {}", exportFilePath.string()));
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
