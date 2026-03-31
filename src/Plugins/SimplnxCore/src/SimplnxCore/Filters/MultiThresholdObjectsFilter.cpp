#include "MultiThresholdObjectsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/MultiThresholdObjectsDirect.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DataTypeParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
Result<> CheckComponentIndicesInThresholds(const ArrayThresholdSet& thresholds, const DataStructure& dataStructure)
{
  Result<> finalResult;
  for(const auto& threshold : thresholds.getArrayThresholds())
  {
    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      Result<> result = CheckComponentIndicesInThresholds(*comparisonSet, dataStructure);
      finalResult = MergeResults(std::move(result), std::move(finalResult));
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      DataPath dataPath = comparisonValue->getArrayPath();
      const auto& currentDataArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
      usize index = comparisonValue->getComponentIndex();
      usize numComponents = currentDataArray.getNumberOfComponents();
      if(index >= currentDataArray.getNumberOfComponents())
      {
        finalResult = MergeResults(MakeErrorResult(to_underlying(MultiThresholdObjectsFilter::ErrorCodes::InvalidComponentIndex),
                                                   fmt::format("Array '{}' has {} component(s) but index {} was selected", dataPath.toString(), numComponents, index)),
                                   std::move(finalResult));
      }
    }
  }
  return finalResult;
}

struct CheckCustomValueInBounds
{
  template <typename T>
  Result<> operator()(float64 customValue)
  {
    float64 minValue;
    float64 maxValue;
    if constexpr(std::is_floating_point_v<T>)
    {
      // Floating Point Types
      minValue = static_cast<float64>(-std::numeric_limits<T>::max());
      maxValue = static_cast<float64>(std::numeric_limits<T>::max());
    }
    else
    {
      // Everything Else
      minValue = static_cast<float64>(std::numeric_limits<T>::min());
      maxValue = static_cast<float64>(std::numeric_limits<T>::max());
    }

    if(customValue < minValue || customValue > maxValue)
    {
      return MakeErrorResult(-100, "Custom value is outside the bounds of the chosen data type!");
    }

    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
std::string MultiThresholdObjectsFilter::name() const
{
  return FilterTraits<MultiThresholdObjectsFilter>::name;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjectsFilter::className() const
{
  return FilterTraits<MultiThresholdObjectsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MultiThresholdObjectsFilter::uuid() const
{
  return FilterTraits<MultiThresholdObjectsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjectsFilter::humanName() const
{
  return "Multi-Threshold Objects";
}

//------------------------------------------------------------------------------
std::vector<std::string> MultiThresholdObjectsFilter::defaultTags() const
{
  return {className(), "Find Outliers", "Threshold", "Isolate", "Data Management"};
}

//------------------------------------------------------------------------------
Parameters MultiThresholdObjectsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ArrayThresholdsParameter>(k_ArrayThresholdsObject_Key, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{},
                                                           ArrayThresholdsParameter::AllowedComponentShapes{}));
  params.insert(std::make_unique<DataTypeParameter>(k_CreatedMaskType_Key, "Mask Type", "DataType used for the created Mask Array", DataType::uint8));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseCustomTrueValue, "Use Custom TRUE Value", "Specifies whether to output a custom TRUE value (the default value is 1)", false));
  params.insert(std::make_unique<NumberParameter<float64>>(k_CustomTrueValue, "Custom TRUE Value", "This is the custom TRUE value that will be output to the mask array", 1.0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseCustomFalseValue, "Use Custom FALSE Value", "Specifies whether to output a custom FALSE value (the default value is 0)", false));
  params.insert(std::make_unique<NumberParameter<float64>>(k_CustomFalseValue, "Custom FALSE Value", "This is the custom FALSE value that will be output to the mask array", 0.0));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedDataName_Key, "Mask Array", "DataPath to the created Mask Array", "Mask"));

  params.linkParameters(k_UseCustomTrueValue, k_CustomTrueValue, true);
  params.linkParameters(k_UseCustomFalseValue, k_CustomFalseValue, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType MultiThresholdObjectsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MultiThresholdObjectsFilter::clone() const
{
  return std::make_unique<MultiThresholdObjectsFilter>();
}

// -----------------------------------------------------------------------------
IFilter::PreflightResult MultiThresholdObjectsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto thresholdsObject = filterArgs.value<ArrayThresholdSet>(k_ArrayThresholdsObject_Key);
  auto maskArrayName = filterArgs.value<std::string>(k_CreatedDataName_Key);
  auto maskArrayType = filterArgs.value<DataType>(k_CreatedMaskType_Key);
  auto useCustomTrueValue = filterArgs.value<BoolParameter::ValueType>(k_UseCustomTrueValue);
  auto useCustomFalseValue = filterArgs.value<BoolParameter::ValueType>(k_UseCustomFalseValue);
  auto customTrueValue = filterArgs.value<NumberParameter<float64>::ValueType>(k_CustomTrueValue);
  auto customFalseValue = filterArgs.value<NumberParameter<float64>::ValueType>(k_CustomFalseValue);

  auto thresholdPaths = thresholdsObject.getRequiredPaths();
  // If the paths are empty just return now.
  if(thresholdPaths.empty())
  {
    return MakePreflightErrorResult(-4000, "No data arrays were found for calculating threshold");
  }

  DataPath firstDataPath = *(thresholdPaths.begin());
  const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(firstDataPath);

  // Check for same number of tuples and components
  usize numTuples = dataArray.getNumberOfTuples();
  usize numComponents = dataArray.getNumberOfComponents();
  for(const auto& dataPath : thresholdPaths)
  {
    const auto& currentDataArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
    usize currentNumTuples = currentDataArray.getNumberOfTuples();
    if(currentNumTuples != numTuples)
    {
      auto errorMessage = fmt::format("Data Arrays do not have same equal number of tuples. '{}:{}' and '{}:{}'", firstDataPath.toString(), numTuples, dataPath.toString(), currentNumTuples);
      return MakePreflightErrorResult(to_underlying(ErrorCodes::UnequalTuples), errorMessage);
    }
    usize currentNumComponents = currentDataArray.getNumberOfComponents();
    if(currentNumComponents != numComponents)
    {
      auto errorMessage =
          fmt::format("Data Arrays do not have same equal number of components. '{}:{}' and '{}:{}'", firstDataPath.toString(), numComponents, dataPath.toString(), currentNumComponents);
      return MakePreflightErrorResult(to_underlying(ErrorCodes::UnequalComponents), errorMessage);
    }
  }

  Result<> componentIndicesResult = CheckComponentIndicesInThresholds(thresholdsObject, dataStructure);
  if(componentIndicesResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(componentIndicesResult))};
  }

  if(maskArrayType == DataType::boolean)
  {
    if(useCustomTrueValue)
    {
      return MakePreflightErrorResult(to_underlying(ErrorCodes::CustomTrueWithBoolean), "Cannot use custom TRUE value with a boolean Mask Type.");
    }

    if(useCustomFalseValue)
    {
      return MakePreflightErrorResult(to_underlying(ErrorCodes::CustomFalseWithBoolean), "Cannot use custom FALSE value with a boolean Mask Type.");
    }
  }

  if(useCustomTrueValue)
  {
    Result<> result = ExecuteDataFunction(CheckCustomValueInBounds{}, maskArrayType, customTrueValue);
    if(result.invalid())
    {
      auto errorMessage = fmt::format("Custom TRUE value ({}) is outside the bounds of the chosen Mask Type ({}).", customTrueValue, DataTypeToString(maskArrayType));
      return MakePreflightErrorResult(to_underlying(ErrorCodes::CustomTrueOutOfBounds), errorMessage);
    }
  }

  if(useCustomFalseValue)
  {
    Result<> result = ExecuteDataFunction(CheckCustomValueInBounds{}, maskArrayType, customFalseValue);
    if(result.invalid())
    {
      auto errorMessage = fmt::format("Custom FALSE value ({}) is outside the bounds of the chosen Mask Type ({}).", customFalseValue, DataTypeToString(maskArrayType));
      return MakePreflightErrorResult(to_underlying(ErrorCodes::CustomFalseOutOfBounds), errorMessage);
    }
  }

  // Create the output boolean array
  auto action =
      std::make_unique<CreateArrayAction>(maskArrayType, dataArray.getIDataStoreRef().getTupleShape(), std::vector<usize>{1}, firstDataPath.replaceName(maskArrayName), dataArray.getDataFormat());

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

// -----------------------------------------------------------------------------
Result<> MultiThresholdObjectsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  MultiThresholdObjectsInputValues inputValues;
  inputValues.ArrayThresholdsObject = filterArgs.value<ArrayThresholdsParameter::ValueType>(k_ArrayThresholdsObject_Key);
  inputValues.CreatedMaskType = filterArgs.value<DataTypeParameter::ValueType>(k_CreatedMaskType_Key);
  inputValues.CustomFalseValue = filterArgs.value<Float64Parameter::ValueType>(k_CustomFalseValue);
  inputValues.CustomTrueValue = filterArgs.value<Float64Parameter::ValueType>(k_CustomTrueValue);
  inputValues.OutputDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedDataName_Key);
  inputValues.UseCustomFalseValue = filterArgs.value<BoolParameter::ValueType>(k_UseCustomFalseValue);
  inputValues.UseCustomTrueValue = filterArgs.value<BoolParameter::ValueType>(k_UseCustomTrueValue);

  return MultiThresholdObjectsDirect(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedThresholdsKey = "SelectedThresholds";
constexpr StringLiteral k_ScalarTypeKey = "ScalarType";
constexpr StringLiteral k_DestinationArrayNameKey = "DestinationArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> MultiThresholdObjectsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MultiThresholdObjectsFilter().getDefaultArguments();
  static constexpr StringLiteral k_FilterUuidKey = "Filter_Uuid";
  static constexpr StringLiteral k_FilterClassNameKey = "Filter_Name";
  static constexpr StringLiteral v1Uuid = "{014b7300-cf36-5ede-a751-5faf9b119dae}";
  static constexpr StringLiteral v1ClassName = "MultiThresholdObjects";

  std::vector<Result<>> results;

  bool isAdvanced = false;
  if(json.contains(k_FilterUuidKey))
  {
    isAdvanced = json[k_FilterUuidKey].get<std::string>() != v1Uuid;
  }
  else if(json.contains(k_FilterClassNameKey))
  {
    // SIMPL 6.4 pipelines do not include Filter_Uuid; fall back to the legacy class name.
    isAdvanced = json[k_FilterClassNameKey].get<std::string>() != v1ClassName;
  }
  if(isAdvanced)
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ComparisonSelectionAdvancedFilterParameterConverter>(args, json, SIMPL::k_SelectedThresholdsKey, k_ArrayThresholdsObject_Key));
  }
  else
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ComparisonSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedThresholdsKey, k_ArrayThresholdsObject_Key));
  }
  Result<> scalarResult = SIMPLConversion::ConvertParameter<SIMPLConversion::ScalarTypeParameterConverter>(args, json, SIMPL::k_ScalarTypeKey, k_CreatedMaskType_Key);
  if(scalarResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(scalarResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_DestinationArrayNameKey, k_CreatedDataName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
