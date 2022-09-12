#include "ConvertOrientations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"
#include "EbsdLib/OrientationMath/OrientationConverter.hpp"

#include <fmt/format.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
using namespace complex;

namespace
{

// Error Code constants
constexpr complex::int32 k_InputRepresentationTypeError = -67001;
constexpr complex::int32 k_OutputRepresentationTypeError = -67002;
constexpr complex::int32 k_InputComponentDimensionError = -67003;
constexpr complex::int32 k_InputComponentCountError = -67004;

// Some constants for the Orientation Representations
const std::vector<std::string> RepresentationNames = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric"};

const std::vector<size_t> RepresentationElementCount = {3, 9, 4, 4, 4, 3, 3};

/**
 *
 */
template <typename T, typename TransformFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class ConvertOrientation
{
public:
  ConvertOrientation(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc)
  : m_inputArray(inputArray)
  , m_outputArray(outputArray)
  , m_transformFunc(std::move(transformFunc))
  {
  }

  void operator()(const ComplexRange& range) const
  {
    auto& inDataStore = m_inputArray.getDataStoreRef();
    auto& outDataStore = m_outputArray.getDataStoreRef();

    Orientation<T> input(InCompSize);
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {

      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }
      Orientation<T> output = m_transformFunc(input); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_inputArray;
  DataArray<T>& m_outputArray;
  TransformFunc m_transformFunc;
};

/**
 *
 */
template <typename T, typename TransformFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class ToQuaternion
{
public:
  ToQuaternion(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, typename Quaternion<T>::Order layout)
  : m_inputArray(inputArray)
  , m_outputArray(outputArray)
  , m_transformFunc(std::move(transformFunc))
  , m_layout(layout)
  {
  }

  void operator()(const ComplexRange& range) const
  {
    using QuaterionType = Quaternion<float>;
    size_t numTuples = m_inputArray.getNumberOfTuples();
    auto& inDataStore = m_inputArray.getDataStoreRef();
    auto& outDataStore = m_outputArray.getDataStoreRef();

    Orientation<T> input(InCompSize);
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {
      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }
      QuaterionType output = m_transformFunc(input, m_layout); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_inputArray;
  DataArray<T>& m_outputArray;
  TransformFunc m_transformFunc;
  typename Quaternion<T>::Order m_layout;
};

/**
 *
 */
template <typename T, typename TransformFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class FromQuaternion
{
public:
  FromQuaternion(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, typename Quaternion<T>::Order layout)
  : m_inputArray(inputArray)
  , m_outputArray(outputArray)
  , m_transformFunc(std::move(transformFunc))
  , m_layout(layout)
  {
  }

  void operator()(const ComplexRange& range) const
  {
    using QuaterionType = Quaternion<T>;
    auto& inDataStore = m_inputArray.getDataStoreRef();
    auto& outDataStore = m_outputArray.getDataStoreRef();

    QuaterionType input;
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {
      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }
      Orientation<T> output = m_transformFunc(input, m_layout); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_inputArray;
  DataArray<T>& m_outputArray;
  TransformFunc m_transformFunc;
  typename Quaternion<T>::Order m_layout;
};

} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertOrientations::name() const
{
  return FilterTraits<ConvertOrientations>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertOrientations::className() const
{
  return FilterTraits<ConvertOrientations>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertOrientations::uuid() const
{
  return FilterTraits<ConvertOrientations>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertOrientations::humanName() const
{
  return "Convert Orientation Representation";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertOrientations::defaultTags() const
{
  return {"Processing", "Conversion", "Orientation", "Quaternions", "Euler Angles", "Orientation Matrix", "Cubochoric", "Homochoric", "Rodrigues", "AxisAngle"};
}

//------------------------------------------------------------------------------
Parameters ConvertOrientations::parameters() const
{
  using OrientationConverterType = OrientationConverter<EbsdDataArray<float>, float>;

  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_InputType_Key, "Input Orientation Type", "", 0, OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Orientation Type", "", 1, OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath_Key, "Input Orientations", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputOrientationArrayName_Key, "Output Orientations", "", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertOrientations::clone() const
{
  return std::make_unique<ConvertOrientations>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertOrientations::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto inputType = static_cast<OrientationRepresentation::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  auto outputType = static_cast<OrientationRepresentation::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key));

  if(static_cast<int>(inputType) < 0 || inputType >= OrientationRepresentation::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(::k_InputRepresentationTypeError, fmt::format("Input Representation Type must be a value from 0 to 6. '{}'", inputType))};
  }

  if(static_cast<int>(outputType) < 0 || outputType >= OrientationRepresentation::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(::k_OutputRepresentationTypeError, fmt::format("Output Representation Type must be a value from 0 to 6. '{}'", outputType))};
  }

  auto pInputArrayPath = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  const auto* inputArray = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  std::vector<size_t> inputCompShape = inputArray->getIDataStore()->getComponentShape();

  if(inputCompShape.size() > 1)
  {
    return {MakeErrorResult<OutputActions>(::k_InputComponentDimensionError, fmt::format("Input Component Shape has multiple dimensions. It can only have 1 dimension. '{}'", inputCompShape.size()))};
  }
  if(inputCompShape[0] != ::RepresentationElementCount[static_cast<size_t>(inputType)])
  {
    std::stringstream message;
    message << "Number of components for input array is not correct for input representation type. " << ::RepresentationNames[static_cast<size_t>(inputType)] << " should have "
            << ::RepresentationElementCount[static_cast<size_t>(inputType)] << " components but the selected input array has " << inputCompShape[0];
    return {MakeErrorResult<OutputActions>(::k_InputComponentCountError, message.str())};
  }
  auto pOutputArrayPath = pInputArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputOrientationArrayName_Key));

  auto numericType = static_cast<DataType>(inputArray->getDataType());
  std::vector<size_t> componentDims = {::RepresentationElementCount[static_cast<size_t>(outputType)]};
  auto action = std::make_unique<CreateArrayAction>(numericType, inputArray->getIDataStore()->getTupleShape(), componentDims, pOutputArrayPath);

  // Create the Output actions
  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ConvertOrientations::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto inputType = static_cast<OrientationRepresentation::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  auto outputType = static_cast<OrientationRepresentation::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key));
  auto pInputOrientationArrayPathValue = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  auto pOutputOrientationArrayNameValue = pInputOrientationArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_OutputOrientationArrayName_Key));

  // Quaternion<float>::Order qLayout = Quaternion<float>::Order::VectorScalar;

  using OutputType = Orientation<float>;
  using InputType = Orientation<float>;
  using QuaterionType = Quaternion<float>;
  using QuaternionType = Quaternion<float>;
  using ConversionFunctionType = std::function<OutputType(InputType)>;
  using ToQuaternionFunctionType = std::function<QuaterionType(InputType, Quaternion<float>::Order)>;
  using FromQuaternionFunctionType = std::function<InputType(QuaterionType, Quaternion<float>::Order)>;

  const auto& inputDataArray = dataStructure.getDataRefAs<Float32Array>(pInputOrientationArrayPathValue);
  auto& outputDataArray = dataStructure.getDataRefAs<Float32Array>(pOutputOrientationArrayNameValue);
  size_t totalPoints = inputDataArray.getNumberOfTuples();

  // Allow data-based parallelization
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, totalPoints);
  // This next block of code was generated from the ConvertOrientationsTest::_make_code() function.
  if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to OrientationMatrix"});
    ConversionFunctionType eu2om = OrientationTransformation::eu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 9>(inputDataArray, outputDataArray, eu2om));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Quaternion"});
    ToQuaternionFunctionType eu2qu = OrientationTransformation::eu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to AxisAngle"});
    ConversionFunctionType eu2ax = OrientationTransformation::eu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2ax));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Rodrigues"});
    ConversionFunctionType eu2ro = OrientationTransformation::eu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2ro));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Homochoric"});
    ConversionFunctionType eu2ho = OrientationTransformation::eu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, eu2ho));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Cubochoric"});
    ConversionFunctionType eu2cu = OrientationTransformation::eu2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, eu2cu));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Euler"});
    ConversionFunctionType om2eu = OrientationTransformation::om2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 9, 3>(inputDataArray, outputDataArray, om2eu));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Quaternion"});
    ToQuaternionFunctionType om2qu = OrientationTransformation::om2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 9, 4>(inputDataArray, outputDataArray, om2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to AxisAngle"});
    ConversionFunctionType om2ax = OrientationTransformation::om2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 9, 4>(inputDataArray, outputDataArray, om2ax));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Rodrigues"});
    ConversionFunctionType om2ro = OrientationTransformation::om2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 9, 4>(inputDataArray, outputDataArray, om2ro));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Homochoric"});
    ConversionFunctionType om2ho = OrientationTransformation::om2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 9, 3>(inputDataArray, outputDataArray, om2ho));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Cubochoric"});
    ConversionFunctionType om2cu = OrientationTransformation::om2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 9, 3>(inputDataArray, outputDataArray, om2cu));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Euler"});
    FromQuaternionFunctionType qu2eu = OrientationTransformation::qu2eu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2eu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to OrientationMatrix"});
    FromQuaternionFunctionType qu2om = OrientationTransformation::qu2om<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 9>(inputDataArray, outputDataArray, qu2om, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to AxisAngle"});
    FromQuaternionFunctionType qu2ax = OrientationTransformation::qu2ax<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 4>(inputDataArray, outputDataArray, qu2ax, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Rodrigues"});
    FromQuaternionFunctionType qu2ro = OrientationTransformation::qu2ro<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 4>(inputDataArray, outputDataArray, qu2ro, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Homochoric"});
    FromQuaternionFunctionType qu2ho = OrientationTransformation::qu2ho<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2ho, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Cubochoric"});
    FromQuaternionFunctionType qu2cu = OrientationTransformation::qu2cu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2cu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Euler"});
    ConversionFunctionType ax2eu = OrientationTransformation::ax2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2eu));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to OrientationMatrix"});
    ConversionFunctionType ax2om = OrientationTransformation::ax2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 9>(inputDataArray, outputDataArray, ax2om));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Quaternion"});
    ToQuaternionFunctionType ax2qu = OrientationTransformation::ax2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 4, 4>(inputDataArray, outputDataArray, ax2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Rodrigues"});
    ConversionFunctionType ax2ro = OrientationTransformation::ax2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 4>(inputDataArray, outputDataArray, ax2ro));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Homochoric"});
    ConversionFunctionType ax2ho = OrientationTransformation::ax2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2ho));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Cubochoric"});
    ConversionFunctionType ax2cu = OrientationTransformation::ax2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2cu));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Euler"});
    ConversionFunctionType ro2eu = OrientationTransformation::ro2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2eu));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to OrientationMatrix"});
    ConversionFunctionType ro2om = OrientationTransformation::ro2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 9>(inputDataArray, outputDataArray, ro2om));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Quaternion"});
    ToQuaternionFunctionType ro2qu = OrientationTransformation::ro2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 4, 4>(inputDataArray, outputDataArray, ro2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to AxisAngle"});
    ConversionFunctionType ro2ax = OrientationTransformation::ro2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 4>(inputDataArray, outputDataArray, ro2ax));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Homochoric"});
    ConversionFunctionType ro2ho = OrientationTransformation::ro2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2ho));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Cubochoric"});
    ConversionFunctionType ro2cu = OrientationTransformation::ro2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2cu));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Euler"});
    ConversionFunctionType ho2eu = OrientationTransformation::ho2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, ho2eu));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to OrientationMatrix"});
    ConversionFunctionType ho2om = OrientationTransformation::ho2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 9>(inputDataArray, outputDataArray, ho2om));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Quaternion"});
    ToQuaternionFunctionType ho2qu = OrientationTransformation::ho2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to AxisAngle"});
    ConversionFunctionType ho2ax = OrientationTransformation::ho2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2ax));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Rodrigues"});
    ConversionFunctionType ho2ro = OrientationTransformation::ho2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2ro));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Cubochoric"});
    ConversionFunctionType ho2cu = OrientationTransformation::ho2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, ho2cu));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Euler"});
    ConversionFunctionType cu2eu = OrientationTransformation::cu2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, cu2eu));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to OrientationMatrix"});
    ConversionFunctionType cu2om = OrientationTransformation::cu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 9>(inputDataArray, outputDataArray, cu2om));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Quaternion"});
    ToQuaternionFunctionType cu2qu = OrientationTransformation::cu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2qu, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to AxisAngle"});
    ConversionFunctionType cu2ax = OrientationTransformation::cu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2ax));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Rodrigues"});
    ConversionFunctionType cu2ro = OrientationTransformation::cu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2ro));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Homochoric"});
    ConversionFunctionType cu2ho = OrientationTransformation::cu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, 3, 3>(inputDataArray, outputDataArray, cu2ho));
  }

  return {};
}
} // namespace complex

#pragma clang diagnostic pop
