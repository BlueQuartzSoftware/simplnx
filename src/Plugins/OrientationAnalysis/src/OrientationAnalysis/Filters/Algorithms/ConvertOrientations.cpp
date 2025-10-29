#include "ConvertOrientations.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

// #include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationRepresentation.h"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include <EbsdLib/Utilities/EbsdStringUtils.hpp>

#ifndef _MSC_VER
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
#endif

using namespace nx::core;

namespace
{
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

// -----------------------------------------------------------------------------
ConvertOrientations::ConvertOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertOrientations::~ConvertOrientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertOrientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertOrientations::operator()()
{
  // Quaternion<float>::Order qLayout = Quaternion<float>::Order::VectorScalar;

  using OutputType = Orientation<float32>;
  using InputType = Orientation<float32>;
  using QuaterionType = Quaternion<float32>;
  using QuaternionType = Quaternion<float32>;
  using ConversionFunctionType = std::function<OutputType(InputType)>;
  using ValidateInputDataFunctionType = std::function<void(float32*)>;
  using ToQuaternionFunctionType = std::function<QuaterionType(InputType, Quaternion<float>::Order)>;
  using FromQuaternionFunctionType = std::function<InputType(QuaterionType, Quaternion<float>::Order)>;

  auto inputArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationArrayPath);
  auto outputArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputOrientationArrayPath);
  size_t totalPoints = inputArray.getNumberOfTuples();

  const ValidateInputDataFunctionType euCheck = EulerCheck<float32>();
  const ValidateInputDataFunctionType omCheck = OrientationMatrixCheck<float32>();
  const ValidateInputDataFunctionType quCheck = QuaternionCheck<float32>();
  const ValidateInputDataFunctionType axCheck = AxisAngleCheck<float32>();
  const ValidateInputDataFunctionType roCheck = RodriguesCheck<float32>();
  const ValidateInputDataFunctionType hoCheck = HomochoricCheck<float32>();
  const ValidateInputDataFunctionType cuCheck = CubochoricCheck<float32>();
  const ValidateInputDataFunctionType stCheck = StereographicCheck<float32>();

  // This next block of code was generated from the ConvertOrientationsTest::_make_code() function.
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, totalPoints);
  if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to OrientationMatrix"});
    ConversionFunctionType eu2om = OrientationTransformation::eu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float32, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputArray, outputArray, eu2om, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to Quaternion"});
    ToQuaternionFunctionType eu2qu = OrientationTransformation::eu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, eu2qu, euCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to AxisAngle"});
    ConversionFunctionType eu2ax = OrientationTransformation::eu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, eu2ax, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to Rodrigues"});
    ConversionFunctionType eu2ro = OrientationTransformation::eu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, eu2ro, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to Homochoric"});
    ConversionFunctionType eu2ho = OrientationTransformation::eu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, eu2ho, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to Cubochoric"});
    ConversionFunctionType eu2cu = OrientationTransformation::eu2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, eu2cu, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Euler && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Euler to Stereographic"});
    ConversionFunctionType eu2st = OrientationTransformation::eu2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, eu2st, euCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Euler"});
    ConversionFunctionType om2eu = OrientationTransformation::om2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputArray, outputArray, om2eu, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Quaternion"});
    ToQuaternionFunctionType om2qu = OrientationTransformation::om2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputArray, outputArray, om2qu, omCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to AxisAngle"});
    ConversionFunctionType om2ax = OrientationTransformation::om2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputArray, outputArray, om2ax, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Rodrigues"});
    ConversionFunctionType om2ro = OrientationTransformation::om2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 4>(inputArray, outputArray, om2ro, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Homochoric"});
    ConversionFunctionType om2ho = OrientationTransformation::om2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputArray, outputArray, om2ho, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Cubochoric"});
    ConversionFunctionType om2cu = OrientationTransformation::om2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputArray, outputArray, om2cu, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::OrientationMatrix && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting OrientationMatrix to Stereographic"});
    ConversionFunctionType om2st = OrientationTransformation::om2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 9, 3>(inputArray, outputArray, om2st, omCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to Euler"});
    FromQuaternionFunctionType qu2eu = OrientationTransformation::qu2eu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, qu2eu, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to OrientationMatrix"});
    FromQuaternionFunctionType qu2om = OrientationTransformation::qu2om<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputArray, outputArray, qu2om, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to AxisAngle"});
    FromQuaternionFunctionType qu2ax = OrientationTransformation::qu2ax<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, qu2ax, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to Rodrigues"});
    FromQuaternionFunctionType qu2ro = OrientationTransformation::qu2ro<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, qu2ro, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to Homochoric"});
    FromQuaternionFunctionType qu2ho = OrientationTransformation::qu2ho<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, qu2ho, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to Cubochoric"});
    FromQuaternionFunctionType qu2cu = OrientationTransformation::qu2cu<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, qu2cu, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Quaternion && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Quaternion to Stereographic"});
    FromQuaternionFunctionType qu2st = OrientationTransformation::qu2st<QuaternionType, OutputType>;
    parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, qu2st, quCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Euler"});
    ConversionFunctionType ax2eu = OrientationTransformation::ax2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ax2eu, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to OrientationMatrix"});
    ConversionFunctionType ax2om = OrientationTransformation::ax2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputArray, outputArray, ax2om, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Quaternion"});
    ToQuaternionFunctionType ax2qu = OrientationTransformation::ax2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, ax2qu, axCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Rodrigues"});
    ConversionFunctionType ax2ro = OrientationTransformation::ax2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, ax2ro, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Homochoric"});
    ConversionFunctionType ax2ho = OrientationTransformation::ax2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ax2ho, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Cubochoric"});
    ConversionFunctionType ax2cu = OrientationTransformation::ax2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ax2cu, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::AxisAngle && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting AxisAngle to Stereographic"});
    ConversionFunctionType ax2st = OrientationTransformation::ax2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ax2st, axCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to Euler"});
    ConversionFunctionType ro2eu = OrientationTransformation::ro2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ro2eu, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to OrientationMatrix"});
    ConversionFunctionType ro2om = OrientationTransformation::ro2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 9>(inputArray, outputArray, ro2om, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to Quaternion"});
    ToQuaternionFunctionType ro2qu = OrientationTransformation::ro2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, ro2qu, roCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to AxisAngle"});
    ConversionFunctionType ro2ax = OrientationTransformation::ro2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 4>(inputArray, outputArray, ro2ax, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to Homochoric"});
    ConversionFunctionType ro2ho = OrientationTransformation::ro2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ro2ho, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to Cubochoric"});
    ConversionFunctionType ro2cu = OrientationTransformation::ro2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ro2cu, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Rodrigues && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Rodrigues to Stereographic"});
    ConversionFunctionType ro2st = OrientationTransformation::ro2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 4, 3>(inputArray, outputArray, ro2st, roCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to Euler"});
    ConversionFunctionType ho2eu = OrientationTransformation::ho2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, ho2eu, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to OrientationMatrix"});
    ConversionFunctionType ho2om = OrientationTransformation::ho2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputArray, outputArray, ho2om, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to Quaternion"});
    ToQuaternionFunctionType ho2qu = OrientationTransformation::ho2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, ho2qu, hoCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to AxisAngle"});
    ConversionFunctionType ho2ax = OrientationTransformation::ho2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, ho2ax, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to Rodrigues"});
    ConversionFunctionType ho2ro = OrientationTransformation::ho2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, ho2ro, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to Cubochoric"});
    ConversionFunctionType ho2cu = OrientationTransformation::ho2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, ho2cu, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Homochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Homochoric to Stereographic"});
    ConversionFunctionType ho2st = OrientationTransformation::ho2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, ho2st, hoCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to Euler"});
    ConversionFunctionType cu2eu = OrientationTransformation::cu2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, cu2eu, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to OrientationMatrix"});
    ConversionFunctionType cu2om = OrientationTransformation::cu2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputArray, outputArray, cu2om, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to Quaternion"});
    ToQuaternionFunctionType cu2qu = OrientationTransformation::cu2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, cu2qu, cuCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to AxisAngle"});
    ConversionFunctionType cu2ax = OrientationTransformation::cu2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, cu2ax, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to Rodrigues"});
    ConversionFunctionType cu2ro = OrientationTransformation::cu2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, cu2ro, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to Homochoric"});
    ConversionFunctionType cu2ho = OrientationTransformation::cu2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, cu2ho, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Cubochoric && m_InputValues->OutputType == OrientationRepresentation::Type::Stereographic)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Cubochoric to Stereographic"});
    ConversionFunctionType cu2st = OrientationTransformation::cu2st<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, cu2st, cuCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::Euler)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to Euler"});
    ConversionFunctionType st2eu = OrientationTransformation::st2eu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, st2eu, stCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::OrientationMatrix)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to OrientationMatrix"});
    ConversionFunctionType st2om = OrientationTransformation::st2om<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 9>(inputArray, outputArray, st2om, stCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::Quaternion)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to Quaternion"});
    ToQuaternionFunctionType st2qu = OrientationTransformation::st2qu<InputType, QuaternionType>;
    parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, st2qu, stCheck, QuaternionType::Order::VectorScalar));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::AxisAngle)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to AxisAngle"});
    ConversionFunctionType st2ax = OrientationTransformation::st2ax<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, st2ax, stCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::Rodrigues)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to Rodrigues"});
    ConversionFunctionType st2ro = OrientationTransformation::st2ro<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 4>(inputArray, outputArray, st2ro, stCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::Homochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to Homochoric"});
    ConversionFunctionType st2ho = OrientationTransformation::st2ho<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, st2ho, stCheck));
  }
  else if(m_InputValues->InputType == OrientationRepresentation::Type::Stereographic && m_InputValues->OutputType == OrientationRepresentation::Type::Cubochoric)
  {
    m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Converting Stereographic to Cubochoric"});
    ConversionFunctionType st2cu = OrientationTransformation::st2cu<InputType, OutputType>;
    parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, 3, 3>(inputArray, outputArray, st2cu, stCheck));
  }

  return {};
}
