#include "RobustAutomaticThreshold.hpp"

#include <filesystem>
#include <fstream>

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr StringLiteral k_InputArrayPath = "array_to_threshold";
constexpr StringLiteral k_GradientMagnitudePath = "gradient_array";
constexpr StringLiteral k_ArrayCreationPath = "created_mask_path";

constexpr int64 k_IncorrectOrMissingInputArray = -60;
constexpr int64 k_IncorrectOrMissingMagnitudeArray = -61;
constexpr int64 k_BadArrayCreationPath = -62;

bool isDataArray(const DataObject* dataObject)
{
  return dataObject->getTypeName() == "DataArray";
}

std::vector<usize> getArrayTupleDims(const DataObject* dataObject)
{
  if(!isDataArray(dataObject))
  {
    return {};
  }

  if(auto dataArray = dynamic_cast<const Int8Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const Int16Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const Int32Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const Int64Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }

  if(auto dataArray = dynamic_cast<const UInt8Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const UInt16Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const UInt32Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const UInt64Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }

  if(auto dataArray = dynamic_cast<const USizeArray*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }

  if(auto dataArray = dynamic_cast<const Float32Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }
  if(auto dataArray = dynamic_cast<const Float64Array*>(dataObject))
  {
    return {dataArray->getNumberOfTuples()};
  }

  return {};
}

usize getNumComponents(const DataObject* dataObject)
{
  if(!isDataArray(dataObject))
  {
    return 0;
  }

  if(auto dataArray = dynamic_cast<const Int8Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const Int16Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const Int32Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const Int64Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }

  if(auto dataArray = dynamic_cast<const UInt8Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const UInt16Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const UInt32Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const UInt64Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }

  if(auto dataArray = dynamic_cast<const USizeArray*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }

  if(auto dataArray = dynamic_cast<const Float32Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }
  if(auto dataArray = dynamic_cast<const Float64Array*>(dataObject))
  {
    return dataArray->getNumberOfComponents();
  }

  return 0;
}

template <typename T>
void findThreshold(const DataArray<T>* input, const Float32Array* gradMagPtr, const UInt8Array* maskPtr)
{
  typename std::shared_ptr<IDataStore<T>> inputData = input->getDataStorePtr().lock();
  std::shared_ptr<IDataStore<float>> gradMag = gradMagPtr->getDataStorePtr().lock();
  std::shared_ptr<IDataStore<uint8>> maskStore = maskPtr->getDataStorePtr().lock();

  size_t numTuples = input->getNumberOfTuples();
  float numerator = 0;
  float denominator = 0;

  for(size_t i = 0; i < numTuples; i++)
  {
    numerator += (inputData->getValue(i) * gradMag->getValue(i));
    denominator += gradMag->getValue(i);
  }

  float threshold = numerator / denominator;

  for(size_t i = 0; i < numTuples; i++)
  {
    if(inputData->getValue(i) < threshold)
    {
      maskStore->setValue(i, 0);
    }
    else
    {
      maskStore->setValue(i, 1);
    }
  }
}

void findThreshold(const DataObject* inputObject, const Float32Array* gradMagnitudeArray, const UInt8Array* maskArray)
{
  if(auto inputArray = dynamic_cast<const Int8Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int16Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int32Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Int64Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const UInt8Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt16Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt32Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const UInt64Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const USizeArray*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }

  if(auto inputArray = dynamic_cast<const Float32Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
  }
  if(auto inputArray = dynamic_cast<const Float64Array*>(inputObject))
  {
    findThreshold(inputArray, gradMagnitudeArray, maskArray);
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

Parameters RobustAutomaticThreshold::parameters() const
{
  Parameters params;
  params.insert(
      std::make_unique<DataPathSelectionParameter>(k_InputArrayPath, "Input Array", "DataArray to Threshold", DataPath()));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GradientMagnitudePath, "Gradient Magnitude", "Type to interpret data as", DataPath()));
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
  
  auto inputArrayObject = data.getData(inputArrayPath);
  if(inputArrayObject == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectOrMissingInputArray, "Could not find the input DataArray."}})};
  }
  if(!isDataArray(inputArrayObject))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectOrMissingInputArray, "Target Input DataArray is of an incorrect DataObject type."}})};
  }

  auto gradientArray = data.getDataAs<DataArray<float32>>(gradientArrayPath);
  if(gradientArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectOrMissingMagnitudeArray, "Failed to find DataArray<float32> Gradient array."}})};
  }

  if(data.getData(createdMaskPath) != nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadArrayCreationPath, "Cannot create Mask array as data already exists at the given path"}})};
  }

  auto tupleDims = getArrayTupleDims(inputArrayObject);
  auto numComponents = getNumComponents(inputArrayObject);
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

  auto inputArrayObject = data.getData(inputArrayPath);
  auto gradientArray = data.getDataAs<DataArray<float32>>(gradientArrayPath);
  auto maskArray = data.getDataAs<DataArray<uint8>>(createdMaskPath);

  findThreshold(inputArrayObject, gradientArray, maskArray);

  return {};
}
} // namespace complex
