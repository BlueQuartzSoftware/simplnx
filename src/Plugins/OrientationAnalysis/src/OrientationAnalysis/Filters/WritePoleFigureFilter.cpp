#include "WritePoleFigureFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WritePoleFigure.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WritePoleFigureFilter::name() const
{
  return FilterTraits<WritePoleFigureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WritePoleFigureFilter::className() const
{
  return FilterTraits<WritePoleFigureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WritePoleFigureFilter::uuid() const
{
  return FilterTraits<WritePoleFigureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WritePoleFigureFilter::humanName() const
{
  return "Export Pole Figure Images";
}

//------------------------------------------------------------------------------
std::vector<std::string> WritePoleFigureFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WritePoleFigureFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Output File Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output File Paths", "This is the path to the directory where the pole figures will be created. One file for each phase.",
                                                          fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));

  params.insert(std::make_unique<StringParameter>(k_ImagePrefix_Key, "Image Prefix", "", "Phase_"));
  params.insert(std::make_unique<Int32Parameter>(k_ImageSize_Key, "Image Size (Square Pixels)", "", 512));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<StringParameter>(k_Title_Key, "Figure Title", "The title to place at the top of the Pole Figure", "Figure Title"));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GenerationAlgorithm_Key, "Pole Figure Type", "", 0, ChoicesParameter::Choices{"Color Intensity", "Discrete"}));

  params.insert(std::make_unique<Int32Parameter>(k_LambertSize_Key, "Lambert Image Size (Pixels)", "", 64));
  params.insert(std::make_unique<Int32Parameter>(k_NumColors_Key, "Number of Colors", "", 32));

  // params.insert(std::make_unique<ChoicesParameter>(k_ImageFormat_Key, "Image Format", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"} /* Change this to the proper choices */));
  params.insert(std::make_unique<ChoicesParameter>(k_ImageLayout_Key, "Image Layout", "", 0, ChoicesParameter::Choices{"Horizontal", "Vertical", "Square"}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaterialNameArrayPath_Key, "Material Name", "", DataPath{},
                                                          complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));

  // params.insertSeparator(Parameters::Separator{"Created Output Data"});

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  params.linkParameters(k_GenerationAlgorithm_Key, k_LambertSize_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GenerationAlgorithm_Key, k_NumColors_Key, std::make_any<ChoicesParameter::ValueType>(0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WritePoleFigureFilter::clone() const
{
  return std::make_unique<WritePoleFigureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WritePoleFigureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{

  auto pTitleValue = filterArgs.value<StringParameter::ValueType>(k_Title_Key);
  auto pGenerationAlgorithmValue = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  auto pLambertSizeValue = filterArgs.value<int32>(k_LambertSize_Key);
  auto pNumColorsValue = filterArgs.value<int32>(k_NumColors_Key);
  // auto pImageFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageFormat_Key);
  auto pImageLayoutValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageLayout_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pImagePrefixValue = filterArgs.value<StringParameter::ValueType>(k_ImagePrefix_Key);
  auto pImageSizeValue = filterArgs.value<int32>(k_ImageSize_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Example Output File.", fmt::format("{}/{}_1.pdf", pOutputPathValue.string(), pImagePrefixValue)});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WritePoleFigureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{

  WritePoleFigureInputValues inputValues;

  inputValues.Title = filterArgs.value<StringParameter::ValueType>(k_Title_Key);
  inputValues.GenerationAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  inputValues.LambertSize = filterArgs.value<int32>(k_LambertSize_Key);
  inputValues.NumColors = filterArgs.value<int32>(k_NumColors_Key);
  // inputValues.ImageFormat = filterArgs.value<ChoicesParameter::ValueType>(k_ImageFormat_Key);
  inputValues.ImageLayout = filterArgs.value<ChoicesParameter::ValueType>(k_ImageLayout_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.ImagePrefix = filterArgs.value<StringParameter::ValueType>(k_ImagePrefix_Key);
  inputValues.ImageSize = filterArgs.value<int32>(k_ImageSize_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.MaterialNameArrayPath = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);

  return WritePoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
