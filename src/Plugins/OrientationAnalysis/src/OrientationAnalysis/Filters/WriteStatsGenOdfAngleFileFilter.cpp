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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output angles file path", fs::path("Data/Output/StatsGenODF.txt"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<Float32ParameterType>(k_Weight_Key, "Default Weight", "This value will be used for the Weight column", 1.0f));
  params.insert(std::make_unique<Int32ParameterType>(k_Sigma_Key, "Default Sigma", "This value will be used for the Sigma column", 1));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "The delimiter separating the data", k_SpaceDelimiter,
                                                   ChoicesParameter::ChoicesType{", (comma)", "; (semicolon)", " (space)", ": (colon)", "\\t {tab}"}));
  params.insert(std::make_unique<BoolParameter>(
      k_ConvertToDegrees_Key, "Convert to Degrees",
      "Whether to convert the Euler angles from radians to degrees. If the Euler angles are already in degrees, this option will 'convert' the data again, resulting in garbage orientations!", false));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Only Write Good Elements", "Whether to only write the Euler angles for those elements denoted as true in the supplied mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Used to define Elements as good or bad. Only required if Only Write Good Elements is checked", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteStatsGenOdfAngleFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteStatsGenOdfAngleFileFilter::clone() const
{
  return std::make_unique<WriteStatsGenOdfAngleFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteStatsGenOdfAngleFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                        const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
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
    return {.output_actions =
                MakeErrorResult<OutputActions>(-9402, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteStatsGenOdfAngleFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteStatsGenOdfAngleFileInputValues inputValues;

  inputValues.output_file = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.weight = filterArgs.value<float32>(k_Weight_Key);
  inputValues.sigma = filterArgs.value<int32>(k_Sigma_Key);
  inputValues.delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.convert_to_degrees = filterArgs.value<bool>(k_ConvertToDegrees_Key);
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.cell_euler_angles_array_path = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return WriteStatsGenOdfAngleFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputFileKey = "OutputFile";
constexpr StringLiteralType k_WeightKey = "Weight";
constexpr StringLiteralType k_SigmaKey = "Sigma";
constexpr StringLiteralType k_DelimiterKey = "Delimiter";
constexpr StringLiteralType k_ConvertToDegreesKey = "ConvertToDegrees";
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteStatsGenOdfAngleFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteStatsGenOdfAngleFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_WeightKey, k_Weight_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_SigmaKey, k_Sigma_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DelimiterKey, k_Delimiter_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_ConvertToDegreesKey, k_ConvertToDegrees_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
