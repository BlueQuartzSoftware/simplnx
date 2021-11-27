#include "RobustAutomaticThreshold.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr int32 k_IncorrectOrMissingInputArray = -60;
constexpr int32 k_IncorrectOrMissingMagnitudeArray = -61;
constexpr int32 k_BadArrayCreationPath = -62;
constexpr int32 k_IncorrectInputArrayType = -63;
constexpr int32 k_MismatchedDims = -64;

template <class T>
void FindThreshold(const DataArray<T>& inputArray, const Float32Array& gradMagnitudeArray, UInt8Array& maskArray)
{
  const IDataStore<T>& inputData = inputArray.getDataStoreRef();
  const IDataStore<float32>& gradMag = gradMagnitudeArray.getDataStoreRef();
  IDataStore<uint8>& maskStore = maskArray.getDataStoreRef();

  usize numTuples = inputArray.getNumberOfTuples();
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
      maskStore.setValue(i, 0);
    }
    else
    {
      maskStore.setValue(i, 1);
    }
  }
}

void FindThreshold(const IDataArray& inputObject, const Float32Array& gradMagnitudeArray, UInt8Array& maskArray)
{
  if(auto inputArray = dynamic_cast<const Int8Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int16Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int32Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int64Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const UInt8Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt16Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt32Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt64Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const USizeArray*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const Float32Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Float64Array*>(&inputObject); inputArray != nullptr)
  {
    FindThreshold(*inputArray, gradMagnitudeArray, maskArray);
  }
}
} // namespace

namespace complex
{
std::string RobustAutomaticThreshold::name() const
{
  return FilterTraits<RobustAutomaticThreshold>::name;
}

std::string RobustAutomaticThreshold::className() const
{
  return FilterTraits<RobustAutomaticThreshold>::className;
}

Uuid RobustAutomaticThreshold::uuid() const
{
  return FilterTraits<RobustAutomaticThreshold>::uuid;
}

std::string RobustAutomaticThreshold::humanName() const
{
  return "Robust Automatic Threshold";
}
//------------------------------------------------------------------------------
std::vector<std::string> RobustAutomaticThreshold::defaultTags() const
{
  return {"#DREAM3D Review", "#Threshold"};
}

Parameters RobustAutomaticThreshold::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPath, "Input Array", "DataArray to Threshold", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GradientMagnitudePath, "Gradient Magnitude Data", "", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ArrayCreationPath, "Mask", "Created mask based on Input Array and Gradient Magnitude", DataPath()));

  return params;
}

IFilter::UniquePointer RobustAutomaticThreshold::clone() const
{
  return std::make_unique<RobustAutomaticThreshold>();
}

IFilter::PreflightResult RobustAutomaticThreshold::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto inputArrayPath = args.value<DataPath>(k_InputArrayPath);
  auto gradientArrayPath = args.value<DataPath>(k_GradientMagnitudePath);
  auto createdMaskPath = args.value<DataPath>(k_ArrayCreationPath);

  std::vector<DataPath> dataPaths;

  const auto& inputArray = data.getDataRefAs<IDataArray>(inputArrayPath);
  if(dynamic_cast<const BoolArray*>(&inputArray) != nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType, "Input Attribute Array to threshold cannot be of type bool")};
  }
  dataPaths.push_back(inputArrayPath);

  const auto& gradientArray = data.getDataRefAs<Float32Array>(gradientArrayPath);
  dataPaths.push_back(gradientArrayPath);

  std::vector<usize> tupleDims = {inputArray.getNumberOfTuples()};
  usize numComponents = inputArray.getNumberOfComponents();

  if(!data.validateNumberOfTuples(dataPaths))
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType, "Input array and gradient array have mismatched dimensions")};
  }

  auto action = std::make_unique<CreateArrayAction>(NumericType::uint8, tupleDims, numComponents, createdMaskPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> RobustAutomaticThreshold::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto inputArrayPath = args.value<DataPath>(k_InputArrayPath);
  auto gradientArrayPath = args.value<DataPath>(k_GradientMagnitudePath);
  auto createdMaskPath = args.value<DataPath>(k_ArrayCreationPath);

  const auto& inputArray = data.getDataRefAs<IDataArray>(inputArrayPath);
  const auto& gradientArray = data.getDataRefAs<DataArray<float32>>(gradientArrayPath);
  auto& maskArray = data.getDataRefAs<DataArray<uint8>>(createdMaskPath);

  FindThreshold(inputArray, gradientArray, maskArray);

  return {};
}
} // namespace complex
