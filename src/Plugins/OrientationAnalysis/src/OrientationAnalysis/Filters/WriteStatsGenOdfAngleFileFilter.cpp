#include "WriteStatsGenOdfAngleFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteStatsGenOdfAngleFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteStatsGenOdfAngleFileFilter::name() const
{
  return FilterTraits<WriteStatsGenOdfAngleFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteStatsGenOdfAngleFileFilter::className() const
{
  return FilterTraits<WriteStatsGenOdfAngleFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteStatsGenOdfAngleFileFilter::uuid() const
{
  return FilterTraits<WriteStatsGenOdfAngleFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteStatsGenOdfAngleFileFilter::humanName() const
{
  return "Export StatsGenerator ODF Angle File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStatsGenOdfAngleFileFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteStatsGenOdfAngleFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output angles file path", fs::path("Data/Output/StatsGenODF.txt"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<Float32Parameter>(k_Weight_Key, "Default Weight", "This value will be used for the Weight column", 1.0f));
  params.insert(std::make_unique<Int32Parameter>(k_Sigma_Key, "Default Sigma", "This value will be used for the Sigma column", 1));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "The delimiter separating the data", k_SpaceDelimiter,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", " (space)", ": (colon)", "\\t {tab}"}));
  params.insert(std::make_unique<BoolParameter>(
      k_ConvertToDegrees_Key, "Convert to Degrees",
      "Whether to convert the Euler angles from radians to degrees. If the Euler angles are already in degrees, this option will 'convert' the data again, resulting in garbage orientations!", false));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Only Write Good Elements", "Whether to only write the Euler angles for those elements denoted as true in the supplied mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Used to define Elements as good or bad. Only required if Only Write Good Elements is checked", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteStatsGenOdfAngleFileFilter::clone() const
{
  return std::make_unique<WriteStatsGenOdfAngleFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteStatsGenOdfAngleFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWeightValue = filterArgs.value<float32>(k_Weight_Key);
  auto pSigmaValue = filterArgs.value<int32>(k_Sigma_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pConvertToDegreesValue = filterArgs.value<bool>(k_ConvertToDegrees_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pWeightValue < 1.0f)
  {
    MakePreflightErrorResult(-9400, "The default 'Weight' value should be at least 1.0. Undefined results will occur from this filter.");
  }

  if(pSigmaValue < 1)
  {
    MakePreflightErrorResult(-9401, "The default 'Sigma' value should be at least 1. Undefined results will occur from this filter.");
  }

  std::vector<DataPath> dataArrays = {pCellEulerAnglesArrayPathValue, pCellPhasesArrayPathValue};
  if(pUseGoodVoxelsValue)
  {
    dataArrays.push_back(pGoodVoxelsArrayPathValue);
  }
  if(!dataStructure.validateNumberOfTuples(dataArrays))
  {
    MakePreflightErrorResult(
        -9402,
        "One or more of the input arrays (euler angles, phases, and mask) does not have matching numbers of tuples. Make sure you are selecting cell level arrays from the same attribute matrix.");
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteStatsGenOdfAngleFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  WriteStatsGenOdfAngleFileInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.Weight = filterArgs.value<float32>(k_Weight_Key);
  inputValues.Sigma = filterArgs.value<int32>(k_Sigma_Key);
  inputValues.Delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.ConvertToDegrees = filterArgs.value<bool>(k_ConvertToDegrees_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  return WriteStatsGenOdfAngleFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
