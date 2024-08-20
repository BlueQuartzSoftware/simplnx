#include "RobustAutomaticThresholdFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr int32 k_IncorrectInputArrayType = -2363;
constexpr int32 k_InconsistentTupleCount = -2364;

struct FindThresholdFunctor
{
  template <class T>
  void operator()(const IDataArray* inputObject, const Float32AbstractDataStore& gradMag, BoolAbstractDataStore& maskStore)
  {
    const auto& inputData = inputObject->getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numTuples = inputData.getNumberOfTuples();
    float numerator = 0;
    float denominator = 0;

    for(usize i = 0; i < numTuples; i++)
    {
      numerator += (inputData.getValue(i) * gradMag.getValue(i));
      denominator += gradMag.getValue(i);
    }

    float threshold = numerator / denominator;

    for(usize i = 0; i < numTuples; i++)
    {
      if(inputData.getValue(i) < threshold)
      {
        maskStore.setValue(i, false);
      }
      else
      {
        maskStore.setValue(i, true);
      }
    }
  }
};
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

IFilter::UniquePointer RobustAutomaticThresholdFilter::clone() const
{
  return std::make_unique<RobustAutomaticThresholdFilter>();
}

IFilter::PreflightResult RobustAutomaticThresholdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto inputArrayPath = args.value<DataPath>(k_InputArrayPath_Key);
  auto gradientArrayPath = args.value<DataPath>(k_GradientMagnitudePath_Key);
  auto createdMaskName = args.value<std::string>(k_ArrayCreationName_Key);

  const DataPath createdMaskPath = inputArrayPath.replaceName(createdMaskName);

  std::vector<DataPath> dataPaths;

  // Validate that the input path is NOT a bool array.
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  if(dynamic_cast<const BoolArray*>(&inputArray) != nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType, "Input Data Array to threshold cannot be of type bool")};
  }
  dataPaths.push_back(inputArrayPath);

  // Validate that the Gradient Image is of the correct type
  const DataObject* dataObject = dataStructure.getData(gradientArrayPath);
  if(dynamic_cast<const Float32Array*>(dataObject) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType,
                                           fmt::format("Gradient Data Array must be of type Float. The object at path '{}' is '{}'", gradientArrayPath.toString(), dataObject->getTypeName()))};
  }
  dataPaths.push_back(gradientArrayPath);

  std::vector<usize> tupleDims = {inputArray.getTupleShape()};
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

Result<> RobustAutomaticThresholdFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto inputArrayPath = args.value<DataPath>(k_InputArrayPath_Key);
  auto gradientArrayPath = args.value<DataPath>(k_GradientMagnitudePath_Key);
  auto createdMaskName = args.value<std::string>(k_ArrayCreationName_Key);

  const auto* inputArray = dataStructure.getDataAs<IDataArray>(inputArrayPath);
  const auto& gradientStore = dataStructure.getDataAs<Float32Array>(gradientArrayPath)->getDataStoreRef();
  auto& maskStore = dataStructure.getDataAs<BoolArray>(inputArrayPath.replaceName(createdMaskName))->getDataStoreRef();

  ExecuteNeighborFunction(FindThresholdFunctor{}, inputArray->getDataType(), inputArray, gradientStore, maskStore);

  return {};
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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ArrayCreationName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
