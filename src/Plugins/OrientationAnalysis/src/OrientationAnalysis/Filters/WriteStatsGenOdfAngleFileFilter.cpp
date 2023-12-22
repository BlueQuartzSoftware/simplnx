#include "WriteStatsGenOdfAngleFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteStatsGenOdfAngleFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
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
  return "Write StatsGenerator ODF Angle File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStatsGenOdfAngleFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
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
      std::make_unique<BoolParameter>(k_UseMask_Key, "Only Write Good Elements", "Whether to only write the Euler angles for those elements denoted as true in the supplied mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Used to define Elements as good or bad. Only required if Only Write Good Elements is checked", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

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
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
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
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrays);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-9402, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
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
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return WriteStatsGenOdfAngleFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_WeightKey = "Weight";
constexpr StringLiteral k_SigmaKey = "Sigma";
constexpr StringLiteral k_DelimiterKey = "Delimiter";
constexpr StringLiteral k_ConvertToDegreesKey = "ConvertToDegrees";
constexpr StringLiteral k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteStatsGenOdfAngleFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteStatsGenOdfAngleFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_WeightKey, k_Weight_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_SigmaKey, k_Sigma_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DelimiterKey, k_Delimiter_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ConvertToDegreesKey, k_ConvertToDegrees_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
