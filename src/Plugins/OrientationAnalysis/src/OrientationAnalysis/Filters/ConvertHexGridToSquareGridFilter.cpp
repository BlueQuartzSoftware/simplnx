#include "ConvertHexGridToSquareGridFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"

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
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing", "Specifies the new spacing values", std::vector<float64>{1, 1}, std::vector<std::string>{"X", "Y"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputPath_Key, "Input File", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile, true));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_GeneratedFileList_Key,"Generated File List", "", GeneratedFileListParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Directory", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
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
//  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
//  auto pOutputDataArrayPathValue = pQuaternionDataArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  ConvertHexGridToSquareGridInputValues inputValues;

//  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
//  inputValues.OutputDataArrayPath = inputValues.QuaternionDataArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));
//  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);
//  inputValues.ConversionType = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  return ConvertHexGridToSquareGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
