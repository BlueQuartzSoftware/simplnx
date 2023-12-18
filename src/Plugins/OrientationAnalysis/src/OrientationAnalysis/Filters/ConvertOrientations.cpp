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

#include "complex/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#ifndef _MSC_VER
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
#endif
using namespace complex;

namespace
{

// Error Code constants
constexpr complex::int32 k_InputRepresentationTypeError = -67001;
constexpr complex::int32 k_OutputRepresentationTypeError = -67002;
constexpr complex::int32 k_InputComponentDimensionError = -67003;
constexpr complex::int32 k_InputComponentCountError = -67004;

template <typename T>
struct EulerCheck
{

  void operator()(T* euler) const
  {
    euler[0] = static_cast<T>(std::fmod(euler[0], EbsdLib::Constants::k_2PiD));
    euler[1] = static_cast<T>(std::fmod(euler[1], EbsdLib::Constants::k_PiD));
    euler[2] = static_cast<T>(std::fmod(euler[2], EbsdLib::Constants::k_2PiD));

    if(euler[0] < 0.0)
    {
      euler[0] *= static_cast<T>(-1.0);
    }
    if(euler[1] < 0.0)
    {
      euler[1] *= static_cast<T>(-1.0);
    }
    if(euler[2] < 0.0)
    {
      euler[2] *= static_cast<T>(-1.0);
    }
  }
};

template <typename T>
struct OrientationMatrixCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    OrientationType oaType(inPtr, 9);

    ResultType res = OrientationTransformation::om_check(oaType);
    if(res.result <= 0)
    {
      std::cout << res.msg << std::endl;
      printRepresentation(std::cout, inPtr, std::string("Bad OM"));
    }
  }
  void printRepresentation(std::ostream& out, T* om, const std::string& label = std::string("Om")) const
  {
    out.precision(16);
    out << label << om[0] << '\t' << om[1] << '\t' << om[2] << std::endl;
    out << label << om[3] << '\t' << om[4] << '\t' << om[5] << std::endl;
    out << label << om[6] << '\t' << om[7] << '\t' << om[8] << std::endl;
  }
};

template <typename T>
struct QuaternionCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct AxisAngleCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct RodriguesCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct HomochoricCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct CubochoricCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct StereographicCheck
{
  using OrientationType = Orientation<T>;
  using ResultType = OrientationTransformation::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

/**
 *
 */
template <typename T, typename TransformFunc, typename CheckFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class ConvertOrientation
{
public:
  ConvertOrientation(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, CheckFunc checkFunc)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_TransformFunc(std::move(transformFunc))
  , m_CheckFunc(std::move(checkFunc))
  {
  }

  void operator()(const Range& range) const
  {
    auto& inDataStore = m_InputArray.getDataStoreRef();
    auto& outDataStore = m_OutputArray.getDataStoreRef();

    Orientation<T> input(InCompSize);
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {

      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }

      m_CheckFunc(input.data());

      Orientation<T> output = m_TransformFunc(input); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_InputArray;
  DataArray<T>& m_OutputArray;
  TransformFunc m_TransformFunc;
  CheckFunc m_CheckFunc;
};

/**
 *
 */
template <typename T, typename TransformFunc, typename CheckFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class ToQuaternion
{
public:
  ToQuaternion(DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, CheckFunc checkFunc, typename Quaternion<T>::Order layout)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_TransformFunc(std::move(transformFunc))
  , m_CheckFunc(std::move(checkFunc))
  , m_Layout(layout)
  {
  }

  void operator()(const Range& range) const
  {
    using QuaterionType = Quaternion<float>;
    size_t numTuples = m_InputArray.getNumberOfTuples();
    auto& inDataStore = m_InputArray.getDataStoreRef();
    auto& outDataStore = m_OutputArray.getDataStoreRef();

    Orientation<T> input(InCompSize);
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {
      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }
      m_CheckFunc(input.data());
      QuaterionType output = m_TransformFunc(input, m_Layout); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_InputArray;
  DataArray<T>& m_OutputArray;
  TransformFunc m_TransformFunc;
  CheckFunc m_CheckFunc;
  typename Quaternion<T>::Order m_Layout;
};

/**
 *
 */
template <typename T, typename TransformFunc, typename CheckFunc, size_t InCompSize = 0, size_t OutCompSize = 0>
class FromQuaternion
{
public:
  FromQuaternion(const DataArray<T>& inputArray, DataArray<T>& outputArray, TransformFunc transformFunc, CheckFunc checkFunc, typename Quaternion<T>::Order layout)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_TransformFunc(std::move(transformFunc))
  , m_CheckFunc(std::move(checkFunc))
  , m_Layout(layout)
  {
  }

  void operator()(const Range& range) const
  {
    using QuaterionType = Quaternion<T>;
    auto& inDataStore = m_InputArray.getDataStoreRef();
    auto& outDataStore = m_OutputArray.getDataStoreRef();

    std::array<T, 4> input;
    for(size_t tIndex = range.min(); tIndex < range.max(); tIndex++)
    {
      for(size_t cIndex = 0; cIndex < InCompSize; cIndex++)
      {
        input[cIndex] = inDataStore.getValue(tIndex * InCompSize + cIndex);
      }
      m_CheckFunc(input.data());

      Orientation<T> output = m_TransformFunc(QuaterionType(input[0], input[1], input[2], input[3]), m_Layout); // Do the actual Conversion
      for(size_t cIndex = 0; cIndex < OutCompSize; cIndex++)
      {
        outDataStore.setValue(tIndex * OutCompSize + cIndex, output[cIndex]);
      }
    }
  }

private:
  const DataArray<T>& m_InputArray;
  DataArray<T>& m_OutputArray;
  TransformFunc m_TransformFunc;
  CheckFunc m_CheckFunc;
  typename Quaternion<T>::Order m_Layout;
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
  return {className(), "Processing", "Conversion", "Orientation", "Quaternions", "Euler Angles", "Orientation Matrix", "Cubochoric", "Homochoric", "Rodrigues", "AxisAngle"};
}

//------------------------------------------------------------------------------
Parameters ConvertOrientations::parameters() const
{
  using OrientationConverterType = OrientationConverter<EbsdDataArray<float>, float>;

  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_InputType_Key, "Input Orientation Type", "Specifies the incoming orientation representation", 0,
                                                   OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Orientation Type", "Specifies to which orientation representation to convert the incoming data", 1,
                                                   OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath_Key, "Input Orientations", "The complete path to the incoming orientation representation data array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputOrientationArrayName_Key, "Output Orientations", "The name of the data array with the converted orientation representation", ""));

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
    return {MakeErrorResult<OutputActions>(::k_InputRepresentationTypeError, fmt::format("Input Representation Type must be a value from 0 to 6. '{}'", fmt::underlying(inputType)))};
  }

  if(static_cast<int>(outputType) < 0 || outputType >= OrientationRepresentation::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(::k_OutputRepresentationTypeError, fmt::format("Output Representation Type must be a value from 0 to 6. '{}'", fmt::underlying(outputType)))};
  }

  auto pInputArrayPath = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  const auto* inputArray = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  std::vector<size_t> inputCompShape = inputArray->getIDataStore()->getComponentShape();

  if(inputCompShape.size() > 1)
  {
    return {MakeErrorResult<OutputActions>(::k_InputComponentDimensionError, fmt::format("Input Component Shape has multiple dimensions. It can only have 1 dimension. '{}'", inputCompShape.size()))};
  }
  using OrientationConverterType = OrientationConverter<EbsdDataArray<float>, float>;
  auto representationNames = OrientationConverterType::GetOrientationTypeStrings<std::vector<std::string>>();
  auto representationElementCount = OrientationConverterType::GetComponentCounts<std::vector<size_t>>();

  if(inputCompShape[0] != representationElementCount[static_cast<size_t>(inputType)])
  {
    std::stringstream message;
    message << "Number of components for input array is not correct for input representation type. " << representationNames[static_cast<size_t>(inputType)] << " should have "
            << representationElementCount[static_cast<size_t>(inputType)] << " components but the selected input array has " << inputCompShape[0];
    return {MakeErrorResult<OutputActions>(::k_InputComponentCountError, message.str())};
  }
  auto pOutputArrayPath = pInputArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputOrientationArrayName_Key));

  auto numericType = static_cast<DataType>(inputArray->getDataType());
  std::vector<size_t> componentDims = {representationElementCount[static_cast<size_t>(outputType)]};
  auto action = std::make_unique<CreateArrayAction>(numericType, inputArray->getIDataStore()->getTupleShape(), componentDims, pOutputArrayPath);

  // Create the Output actions
  OutputActions actions;
  actions.appendAction(std::move(action));

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
  using ValidateInputDataFunctionType = std::function<void(float*)>;
  using ToQuaternionFunctionType = std::function<QuaterionType(InputType, Quaternion<float>::Order)>;
  using FromQuaternionFunctionType = std::function<InputType(QuaterionType, Quaternion<float>::Order)>;

  auto& inputDataArray = dataStructure.getDataRefAs<Float32Array>(pInputOrientationArrayPathValue);
  auto& outputDataArray = dataStructure.getDataRefAs<Float32Array>(pOutputOrientationArrayNameValue);
  size_t totalPoints = inputDataArray.getNumberOfTuples();

  const ValidateInputDataFunctionType euCheck = EulerCheck<float>();
  const ValidateInputDataFunctionType omCheck = OrientationMatrixCheck<float>();
  const ValidateInputDataFunctionType quCheck = QuaternionCheck<float>();
  const ValidateInputDataFunctionType axCheck = AxisAngleCheck<float>();
  const ValidateInputDataFunctionType roCheck = RodriguesCheck<float>();
  const ValidateInputDataFunctionType hoCheck = HomochoricCheck<float>();
  const ValidateInputDataFunctionType cuCheck = CubochoricCheck<float>();
  const ValidateInputDataFunctionType stCheck = StereographicCheck<float>();

  // Allow data-based parallelization
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, totalPoints);
  // This next block of code was generated from the ConvertOrientationsTest::_make_code() function.
  if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to OrientationMatrix"});
    ConversionFunctionType eu2om = OrientationTransformation::eu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputDataArray, outputDataArray, eu2om, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Quaternion"});
    ToQuaternionFunctionType eu2qu = OrientationTransformation::eu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2qu, euCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to AxisAngle"});
    ConversionFunctionType eu2ax = OrientationTransformation::eu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2ax, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Rodrigues"});
    ConversionFunctionType eu2ro = OrientationTransformation::eu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, eu2ro, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Homochoric"});
    ConversionFunctionType eu2ho = OrientationTransformation::eu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, eu2ho, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Cubochoric"});
    ConversionFunctionType eu2cu = OrientationTransformation::eu2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, eu2cu, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Euler && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Euler to Stereographic"});
    ConversionFunctionType eu2st = OrientationTransformation::eu2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, eu2st, euCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Euler"});
    ConversionFunctionType om2eu = OrientationTransformation::om2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputDataArray, outputDataArray, om2eu, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Quaternion"});
    ToQuaternionFunctionType om2qu = OrientationTransformation::om2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputDataArray, outputDataArray, om2qu, omCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to AxisAngle"});
    ConversionFunctionType om2ax = OrientationTransformation::om2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputDataArray, outputDataArray, om2ax, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Rodrigues"});
    ConversionFunctionType om2ro = OrientationTransformation::om2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputDataArray, outputDataArray, om2ro, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Homochoric"});
    ConversionFunctionType om2ho = OrientationTransformation::om2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputDataArray, outputDataArray, om2ho, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Cubochoric"});
    ConversionFunctionType om2cu = OrientationTransformation::om2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputDataArray, outputDataArray, om2cu, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::OrientationMatrix && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting OrientationMatrix to Stereographic"});
    ConversionFunctionType om2st = OrientationTransformation::om2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputDataArray, outputDataArray, om2st, omCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Euler"});
    FromQuaternionFunctionType qu2eu = OrientationTransformation::qu2eu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2eu, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to OrientationMatrix"});
    FromQuaternionFunctionType qu2om = OrientationTransformation::qu2om<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputDataArray, outputDataArray, qu2om, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to AxisAngle"});
    FromQuaternionFunctionType qu2ax = OrientationTransformation::qu2ax<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, qu2ax, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Rodrigues"});
    FromQuaternionFunctionType qu2ro = OrientationTransformation::qu2ro<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, qu2ro, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Homochoric"});
    FromQuaternionFunctionType qu2ho = OrientationTransformation::qu2ho<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2ho, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Cubochoric"});
    FromQuaternionFunctionType qu2cu = OrientationTransformation::qu2cu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2cu, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Quaternion && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Quaternion to Stereographic"});
    FromQuaternionFunctionType qu2st = OrientationTransformation::qu2st<QuaternionType, OutputType>;
    parallelAlgorithm.execute(
        ::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, qu2st, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Euler"});
    ConversionFunctionType ax2eu = OrientationTransformation::ax2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2eu, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to OrientationMatrix"});
    ConversionFunctionType ax2om = OrientationTransformation::ax2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputDataArray, outputDataArray, ax2om, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Quaternion"});
    ToQuaternionFunctionType ax2qu = OrientationTransformation::ax2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, ax2qu, axCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Rodrigues"});
    ConversionFunctionType ax2ro = OrientationTransformation::ax2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, ax2ro, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Homochoric"});
    ConversionFunctionType ax2ho = OrientationTransformation::ax2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2ho, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Cubochoric"});
    ConversionFunctionType ax2cu = OrientationTransformation::ax2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2cu, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::AxisAngle && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting AxisAngle to Stereographic"});
    ConversionFunctionType ax2st = OrientationTransformation::ax2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ax2st, axCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Euler"});
    ConversionFunctionType ro2eu = OrientationTransformation::ro2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2eu, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to OrientationMatrix"});
    ConversionFunctionType ro2om = OrientationTransformation::ro2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputDataArray, outputDataArray, ro2om, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Quaternion"});
    ToQuaternionFunctionType ro2qu = OrientationTransformation::ro2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, ro2qu, roCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to AxisAngle"});
    ConversionFunctionType ro2ax = OrientationTransformation::ro2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputDataArray, outputDataArray, ro2ax, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Homochoric"});
    ConversionFunctionType ro2ho = OrientationTransformation::ro2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2ho, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Cubochoric"});
    ConversionFunctionType ro2cu = OrientationTransformation::ro2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2cu, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Rodrigues && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Rodrigues to Stereographic"});
    ConversionFunctionType ro2st = OrientationTransformation::ro2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputDataArray, outputDataArray, ro2st, roCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Euler"});
    ConversionFunctionType ho2eu = OrientationTransformation::ho2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, ho2eu, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to OrientationMatrix"});
    ConversionFunctionType ho2om = OrientationTransformation::ho2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputDataArray, outputDataArray, ho2om, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Quaternion"});
    ToQuaternionFunctionType ho2qu = OrientationTransformation::ho2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2qu, hoCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to AxisAngle"});
    ConversionFunctionType ho2ax = OrientationTransformation::ho2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2ax, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Rodrigues"});
    ConversionFunctionType ho2ro = OrientationTransformation::ho2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, ho2ro, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Cubochoric"});
    ConversionFunctionType ho2cu = OrientationTransformation::ho2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, ho2cu, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Homochoric && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Homochoric to Stereographic"});
    ConversionFunctionType ho2st = OrientationTransformation::ho2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, ho2st, hoCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Euler"});
    ConversionFunctionType cu2eu = OrientationTransformation::cu2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, cu2eu, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to OrientationMatrix"});
    ConversionFunctionType cu2om = OrientationTransformation::cu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputDataArray, outputDataArray, cu2om, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Quaternion"});
    ToQuaternionFunctionType cu2qu = OrientationTransformation::cu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2qu, cuCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to AxisAngle"});
    ConversionFunctionType cu2ax = OrientationTransformation::cu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2ax, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Rodrigues"});
    ConversionFunctionType cu2ro = OrientationTransformation::cu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, cu2ro, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Homochoric"});
    ConversionFunctionType cu2ho = OrientationTransformation::cu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, cu2ho, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Cubochoric && outputType == OrientationRepresentation::Type::Stereographic)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Cubochoric to Stereographic"});
    ConversionFunctionType cu2st = OrientationTransformation::cu2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, cu2st, cuCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::Euler)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to Euler"});
    ConversionFunctionType st2eu = OrientationTransformation::st2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, st2eu, stCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to OrientationMatrix"});
    ConversionFunctionType st2om = OrientationTransformation::st2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputDataArray, outputDataArray, st2om, stCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::Quaternion)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to Quaternion"});
    ToQuaternionFunctionType st2qu = OrientationTransformation::st2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(
        ::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, st2qu, stCheck, QuaternionType::Order::VectorScalar));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::AxisAngle)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to AxisAngle"});
    ConversionFunctionType st2ax = OrientationTransformation::st2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, st2ax, stCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::Rodrigues)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to Rodrigues"});
    ConversionFunctionType st2ro = OrientationTransformation::st2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputDataArray, outputDataArray, st2ro, stCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::Homochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to Homochoric"});
    ConversionFunctionType st2ho = OrientationTransformation::st2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, st2ho, stCheck));
  }
  else if(inputType == OrientationRepresentation::Type::Stereographic && outputType == OrientationRepresentation::Type::Cubochoric)
  {
    messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, "Converting Stereographic to Cubochoric"});
    ConversionFunctionType st2cu = OrientationTransformation::st2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputDataArray, outputDataArray, st2cu, stCheck));
  }

  return {};
}
} // namespace complex

#ifndef _MSC_VER
#pragma clang diagnostic pop
#endif // !_MSVC_

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputTypeKey = "InputType";
constexpr StringLiteral k_OutputTypeKey = "OutputType";
constexpr StringLiteral k_InputOrientationArrayPathKey = "InputOrientationArrayPath";
constexpr StringLiteral k_OutputOrientationArrayNameKey = "OutputOrientationArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ConvertOrientations::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ConvertOrientations().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_InputTypeKey, k_InputType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_OutputTypeKey, k_OutputType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_InputOrientationArrayPathKey, k_InputOrientationArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_OutputOrientationArrayNameKey, k_OutputOrientationArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
