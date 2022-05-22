#include "ConvertOrientations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"
#include "EbsdLib/OrientationMath/OrientationConverter.hpp"

#include <fmt/format.h>

using namespace complex;

namespace
{

// Error Code constants
constexpr complex::int32 k_InputRepresentationTypeError = -67001;
constexpr complex::int32 k_OuputRepresentationTypeError = -67002;
constexpr complex::int32 k_InputComponentDimensionError = -67003;
constexpr complex::int32 k_InputComponentCountError = -67004;

// Some constants for the Orientation Representations
const std::vector<std::string> RepresentationNames = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric"};

const std::vector<size_t> RepresentationElementCount = {3, 9, 4, 4, 4, 3, 3};

/**
 *
 */
template <typename T, typename InputType, size_t InCompSize, typename OutputType, size_t OutCompSize, typename TransformFunc>
class ConvertOrientationImpl
{

public:
  ConvertOrientationImpl(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_TransformFunc(transformFunc)
  {
  }
  virtual ~ConvertOrientationImpl() = default;

  void convert(size_t start, size_t end) const
  {
    auto& inDataStore = m_InputArray.getDataStoreRef();
    auto& outDataStore = m_OutputArray.getDataStoreRef();
    InputType input(InCompSize);
    for(size_t t = start; t < end; t++)
    {
      for(size_t c = 0; c < InCompSize; c++)
      {
        input[c] = inDataStore.getValue(t * InCompSize + c);
      }
      OutputType output = m_TransformFunc(input); // Do the actual Conversion
      for(size_t c = 0; c < OutCompSize; c++)
      {
        outDataStore.setValue(t * OutCompSize + c, output[c]);
      }
    }
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const DataArray<T>& m_InputArray;
  DataArray<T>& m_OutputArray;
  TransformFunc m_TransformFunc;
};

/**
 *
 */
template <typename T, size_t InCompSize = 0, size_t OutCompSize = 0, typename TransformFunc>
void ConvertOrientation(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc)
{
  size_t numTuples = inputArray.getNumberOfTuples();
  auto& inDataStore = inputArray.getDataStoreRef();
  auto& outDataStore = outputArray.getDataStoreRef();

  Orientation<T> input(InCompSize);
  for(size_t t = 0; t < numTuples; t++)
  {

    for(size_t c = 0; c < InCompSize; c++)
    {
      input[c] = inDataStore.getValue(t * InCompSize + c);
    }
    Orientation<T> output = transformFunc(input); // Do the actual Conversion
    for(size_t c = 0; c < OutCompSize; c++)
    {
      outDataStore.setValue(t * OutCompSize + c, output[c]);
    }
  }
}

/**
 *
 */
template <typename T, size_t InCompSize = 0, size_t OutCompSize = 0, typename TransformFunc>
void ToQuaternion(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, typename Quaternion<T>::Order layout)
{
  using QuaterionType = Quaternion<T>;

  size_t numTuples = inputArray.getNumberOfTuples();
  auto& inDataStore = inputArray.getDataStoreRef();
  auto& outDataStore = outputArray.getDataStoreRef();

  Orientation<T> input(InCompSize);
  for(size_t t = 0; t < numTuples; t++)
  {
    for(size_t c = 0; c < InCompSize; c++)
    {
      input[c] = inDataStore.getValue(t * InCompSize + c);
    }
    QuaterionType output = transformFunc(input, layout); // Do the actual Conversion
    for(size_t c = 0; c < OutCompSize; c++)
    {
      outDataStore.setValue(t * OutCompSize + c, output[c]);
    }
  }
}

/**
 *
 */
template <typename T, size_t InCompSize = 0, size_t OutCompSize = 0, typename TransformFunc>
void FromQuaterion(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, typename Quaternion<T>::Order layout)
{
  using QuaterionType = Quaternion<T>;
  size_t numTuples = inputArray.getNumberOfTuples();
  auto& inDataStore = inputArray.getDataStoreRef();
  auto& outDataStore = outputArray.getDataStoreRef();

  QuaterionType input;
  for(size_t t = 0; t < numTuples; t++)
  {
    for(size_t c = 0; c < InCompSize; c++)
    {
      input[c] = inDataStore.getValue(t * InCompSize + c);
    }
    Orientation<T> output = transformFunc(input, layout); // Do the actual Conversion
    for(size_t c = 0; c < OutCompSize; c++)
    {
      outDataStore.setValue(t * OutCompSize + c, output[c]);
    }
  }
}

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
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputOrientationArrayName_Key, "Output Orientations", "", DataPath{}));

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
    return {MakeErrorResult<OutputActions>(::k_OuputRepresentationTypeError, fmt::format("Output Representation Type must be a value from 0 to 6. '{}'", outputType))};
  }

  auto pInputArrayPath = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  auto* inputArray = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  std::vector<size_t> inputCompShape = inputArray->getIDataStore()->getComponentShape();

  if(inputCompShape.size() > 1)
  {
    return {MakeErrorResult<OutputActions>(::k_InputComponentDimensionError, fmt::format("Input Component Shape has mulitple dimensions. It can only have 1 dimension. '{}'", inputCompShape.size()))};
  }
  if(inputCompShape[0] != ::RepresentationElementCount[static_cast<size_t>(inputType)])
  {
    std::stringstream ss;
    ss << "Number of components for input array is not correct for input representation type. " << ::RepresentationNames[static_cast<size_t>(inputType)] << " should have "
       << ::RepresentationElementCount[static_cast<size_t>(inputType)] << " components but the selected input array has " << inputCompShape[0];
    return {MakeErrorResult<OutputActions>(::k_InputComponentCountError, ss.str())};
  }
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

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
  auto pOutputOrientationArrayNameValue = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

  Quaternion<float>::Order qLayout = Quaternion<float>::Order::VectorScalar;

  using OutputType = Orientation<float>;
  using InputType = Orientation<float>;
  using QuaterionType = Quaternion<float>;
  using QuaterionType = Quaternion<float>;
  using ConversionFunctionType = std::function<OutputType(InputType)>;
  using ToQuaterionFunctionType = std::function<QuaterionType(InputType, Quaternion<float>::Order)>;
  using FromQuaternionFunctionType = std::function<InputType(QuaterionType, Quaternion<float>::Order)>;

  const Float32Array& inputDataArray = dataStructure.getDataRefAs<Float32Array>(pInputOrientationArrayPathValue);
  Float32Array& outputDataArray = dataStructure.getDataRefAs<Float32Array>(pOutputOrientationArrayNameValue);

  // Parallel algorithm to find duplicate nodes
  //  ParallelDataAlgorithm dataAlg;
  //  dataAlg.setRange(0ULL, static_cast<size_t>(inputDataArray.getNumberOfTuples()));

  if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    ConversionFunctionType eu2om = OrientationTransformation::eu2om<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 9>(inputDataArray, outputDataArray, eu2om);
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType eu2qu = OrientationTransformation::eu2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 3, 4>(inputDataArray, outputDataArray, eu2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    ConversionFunctionType eu2ax = OrientationTransformation::eu2ax<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, eu2ax);
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    ConversionFunctionType eu2ro = OrientationTransformation::eu2ro<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, eu2ro);
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Homochoric)
  {
    ConversionFunctionType eu2ho = OrientationTransformation::eu2ho<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, eu2ho);
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    ConversionFunctionType eu2cu = OrientationTransformation::eu2cu<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, eu2cu);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Euler)
  {
    ConversionFunctionType om2eu = OrientationTransformation::om2eu<InputType, OutputType>;
    ::ConvertOrientation<float, 9, 3>(inputDataArray, outputDataArray, om2eu);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType om2qu = OrientationTransformation::om2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 9, 4>(inputDataArray, outputDataArray, om2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    ConversionFunctionType om2ax = OrientationTransformation::om2ax<InputType, OutputType>;
    ::ConvertOrientation<float, 9, 4>(inputDataArray, outputDataArray, om2ax);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    ConversionFunctionType om2ro = OrientationTransformation::om2ro<InputType, OutputType>;
    ::ConvertOrientation<float, 9, 4>(inputDataArray, outputDataArray, om2ro);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Homochoric)
  {
    ConversionFunctionType om2ho = OrientationTransformation::om2ho<InputType, OutputType>;
    ::ConvertOrientation<float, 9, 3>(inputDataArray, outputDataArray, om2ho);
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    ConversionFunctionType om2cu = OrientationTransformation::om2cu<InputType, OutputType>;
    ::ConvertOrientation<float, 9, 3>(inputDataArray, outputDataArray, om2cu);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Euler)
  {
    FromQuaternionFunctionType qu2eu = OrientationTransformation::qu2eu<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 3>(inputDataArray, outputDataArray, qu2eu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    FromQuaternionFunctionType qu2om = OrientationTransformation::qu2om<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 9>(inputDataArray, outputDataArray, qu2om, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    FromQuaternionFunctionType qu2ax = OrientationTransformation::qu2ax<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 4>(inputDataArray, outputDataArray, qu2ax, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    FromQuaternionFunctionType qu2ro = OrientationTransformation::qu2ro<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 4>(inputDataArray, outputDataArray, qu2ro, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Homochoric)
  {
    FromQuaternionFunctionType qu2ho = OrientationTransformation::qu2ho<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 3>(inputDataArray, outputDataArray, qu2ho, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    FromQuaternionFunctionType qu2cu = OrientationTransformation::qu2cu<QuaterionType, OutputType>;
    ::FromQuaterion<float, 4, 3>(inputDataArray, outputDataArray, qu2cu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Euler)
  {
    ConversionFunctionType ax2eu = OrientationTransformation::ax2eu<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ax2eu);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    ConversionFunctionType ax2om = OrientationTransformation::ax2om<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 9>(inputDataArray, outputDataArray, ax2om);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType ax2qu = OrientationTransformation::ax2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 4, 4>(inputDataArray, outputDataArray, ax2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    ConversionFunctionType ax2ro = OrientationTransformation::ax2ro<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 4>(inputDataArray, outputDataArray, ax2ro);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Homochoric)
  {
    ConversionFunctionType ax2ho = OrientationTransformation::ax2ho<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ax2ho);
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    ConversionFunctionType ax2cu = OrientationTransformation::ax2cu<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ax2cu);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Euler)
  {
    ConversionFunctionType ro2eu = OrientationTransformation::ro2eu<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ro2eu);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    ConversionFunctionType ro2om = OrientationTransformation::ro2om<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 9>(inputDataArray, outputDataArray, ro2om);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType ro2qu = OrientationTransformation::ro2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 4, 4>(inputDataArray, outputDataArray, ro2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    ConversionFunctionType ro2ax = OrientationTransformation::ro2ax<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 4>(inputDataArray, outputDataArray, ro2ax);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Homochoric)
  {
    ConversionFunctionType ro2ho = OrientationTransformation::ro2ho<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ro2ho);
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    ConversionFunctionType ro2cu = OrientationTransformation::ro2cu<InputType, OutputType>;
    ::ConvertOrientation<float, 4, 3>(inputDataArray, outputDataArray, ro2cu);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    ConversionFunctionType ho2eu = OrientationTransformation::ho2eu<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, ho2eu);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    ConversionFunctionType ho2om = OrientationTransformation::ho2om<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 9>(inputDataArray, outputDataArray, ho2om);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType ho2qu = OrientationTransformation::ho2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 3, 4>(inputDataArray, outputDataArray, ho2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    ConversionFunctionType ho2ax = OrientationTransformation::ho2ax<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, ho2ax);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    ConversionFunctionType ho2ro = OrientationTransformation::ho2ro<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, ho2ro);
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    ConversionFunctionType ho2cu = OrientationTransformation::ho2cu<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, ho2cu);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    ConversionFunctionType cu2eu = OrientationTransformation::cu2eu<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, cu2eu);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    ConversionFunctionType cu2om = OrientationTransformation::cu2om<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 9>(inputDataArray, outputDataArray, cu2om);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    ToQuaterionFunctionType cu2qu = OrientationTransformation::cu2qu<InputType, QuaterionType>;
    ::ToQuaternion<float, 3, 4>(inputDataArray, outputDataArray, cu2qu, QuaterionType::Order::VectorScalar);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    ConversionFunctionType cu2ax = OrientationTransformation::cu2ax<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, cu2ax);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    ConversionFunctionType cu2ro = OrientationTransformation::cu2ro<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 4>(inputDataArray, outputDataArray, cu2ro);
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Homochoric)
  {
    ConversionFunctionType cu2ho = OrientationTransformation::cu2ho<InputType, OutputType>;
    ::ConvertOrientation<float, 3, 3>(inputDataArray, outputDataArray, cu2ho);
  }

  return {};
}
} // namespace complex
