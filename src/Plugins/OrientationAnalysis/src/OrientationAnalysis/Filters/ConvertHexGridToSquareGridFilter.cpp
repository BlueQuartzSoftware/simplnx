#include "ConvertHexGridToSquareGridFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::name() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::className() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertHexGridToSquareGridFilter::uuid() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::humanName() const
{
  return "Convert Hexagonal Grid Data to Square Grid Data (TSL - .ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertHexGridToSquareGridFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertHexGridToSquareGridFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Orientation Source Data"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_MultipleFiles_Key, "Convert Multiple Files", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "Specifies the new spacing values", std::vector<float32>{1.0f, 1.0f}, std::vector<std::string>{"X", "Y"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputPath_Key, "Input File", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile, true));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_GeneratedFileList_Key, "Generated File List", "", GeneratedFileListParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Directory", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_OutputPrefix_Key, "Output Prefix", "", "Sqr_"));

  // Link Parameters
  params.linkParameters(k_MultipleFiles_Key, k_GeneratedFileList_Key, true);
  params.linkParameters(k_MultipleFiles_Key, k_InputPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertHexGridToSquareGridFilter::clone() const
{
  return std::make_unique<ConvertHexGridToSquareGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertHexGridToSquareGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto pUseMultipleFilesValue = filterArgs.value<bool>(k_MultipleFiles_Key);
  auto pInputFileListInfoValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_GeneratedFileList_Key);

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pUseMultipleFilesValue)
  {
    std::vector<std::string> files = pInputFileListInfoValue.generate();

    if(files.empty())
    {
      return {MakeErrorResult<OutputActions>(-1, "GeneratedFileList must not be empty")};
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  ConvertHexGridToSquareGridInputValues inputValues;

  inputValues.MultiFile = filterArgs.value<bool>(k_MultipleFiles_Key);
  inputValues.XYSpacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.OutputFilePrefix = filterArgs.value<std::string>(k_OutputPrefix_Key);
  inputValues.InputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_GeneratedFileList_Key);
  if(inputValues.MultiFile)
  {
    inputValues.InputPath = fs::path(inputValues.InputFileListInfo.inputPath);
  }
  else
  {
    inputValues.InputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputPath_Key);
  }

  return ConvertHexGridToSquareGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
