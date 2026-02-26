#include "RobustAutomaticThresholdFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RobustAutomaticThreshold.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr int32 k_InconsistentTupleCount = -2364;
} // namespace

namespace nx::core
{
std::string RobustAutomaticThresholdFilter::name() const
{
  return FilterTraits<RobustAutomaticThresholdFilter>::name;
}

std::string RobustAutomaticThresholdFilter::className() const
{
  return FilterTraits<RobustAutomaticThresholdFilter>::className;
}

Uuid RobustAutomaticThresholdFilter::uuid() const
{
  return FilterTraits<RobustAutomaticThresholdFilter>::uuid;
}

std::string RobustAutomaticThresholdFilter::humanName() const
{
  return "Robust Automatic Threshold";
}
//------------------------------------------------------------------------------
std::vector<std::string> RobustAutomaticThresholdFilter::defaultTags() const
{
  return {className(), "SimplnxCore", "Threshold"};
}

Parameters RobustAutomaticThresholdFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  // Input cannot be bool array
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPath_Key, "Input Array", "DataArray to Threshold", DataPath(), nx::core::GetAllNumericTypes(),
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GradientMagnitudePath_Key, "Gradient Magnitude Data", "The complete path to the Array specifying the gradient magnitude of the Input Array",
                                                          DataPath(), ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ArrayCreationName_Key, "Mask", "Created mask based on Input Array and Gradient Magnitude", "mask"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RobustAutomaticThresholdFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer RobustAutomaticThresholdFilter::clone() const
{
  return std::make_unique<RobustAutomaticThresholdFilter>();
}

IFilter::PreflightResult RobustAutomaticThresholdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputArrayPath = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto gradientArrayPath = filterArgs.value<DataPath>(k_GradientMagnitudePath_Key);
  auto createdMaskName = filterArgs.value<std::string>(k_ArrayCreationName_Key);

  const DataPath createdMaskPath = inputArrayPath.replaceName(createdMaskName);

  std::vector<DataPath> dataPaths;

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  dataPaths.push_back(inputArrayPath);
  dataPaths.push_back(gradientArrayPath);

  ShapeType tupleDims = {inputArray.getTupleShape()};
  usize numComponents = inputArray.getNumberOfComponents();

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(k_InconsistentTupleCount,
                                           fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  auto action = std::make_unique<CreateArrayAction>(DataType::boolean, tupleDims, std::vector<usize>{numComponents}, createdMaskPath, inputArray.getDataFormat());

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

Result<> RobustAutomaticThresholdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RobustAutomaticThresholdInputValues inputValues;
  inputValues.InputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputArrayPath_Key);
  inputValues.GradientArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_GradientMagnitudePath_Key);
  inputValues.CreatedMaskName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ArrayCreationName_Key);

  return RobustAutomaticThreshold(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputArrayPathKey = "InputArrayPath";
constexpr StringLiteral k_GradientMagnitudeArrayPathKey = "GradientMagnitudeArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> RobustAutomaticThresholdFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RobustAutomaticThresholdFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_InputArrayPathKey, k_InputArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GradientMagnitudeArrayPathKey, k_GradientMagnitudePath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ArrayCreationName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
