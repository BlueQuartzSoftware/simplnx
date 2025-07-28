#include "ConvertOrientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Math/EbsdLibMath.h>
#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>
#include <EbsdLib/Utilities/EbsdStringUtils.hpp>

#include <iostream>
#include <string>

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
    euler[0] = static_cast<T>(std::fmod(euler[0], ebsdlib::constants::k_2PiD));
    euler[1] = static_cast<T>(std::fmod(euler[1], ebsdlib::constants::k_PiD));
    euler[2] = static_cast<T>(std::fmod(euler[2], ebsdlib::constants::k_2PiD));

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
  using OrientationType = ebsdlib::OrientationMatrix<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    OrientationType oaType(inPtr);

    ResultType res = oaType.isValid();
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
  using OrientationType = ebsdlib::Quaternion<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct AxisAngleCheck
{
  using OrientationType = ebsdlib::AxisAngle<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct RodriguesCheck
{
  using OrientationType = ebsdlib::Rodrigues<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct HomochoricCheck
{
  using OrientationType = ebsdlib::Homochoric<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct CubochoricCheck
{
  using OrientationType = ebsdlib::Cubochoric<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

template <typename T>
struct StereographicCheck
{
  using OrientationType = ebsdlib::Stereographic<T>;
  using ResultType = ebsdlib::ResultType;

  void operator()(T* inPtr) const
  {
    // This is a no-op at this point.
  }
};

#define OC_TBB_IMPL(TO_REP)                                                                                                                                                                            \
  template <typename T, typename K, class InputType, class OutputType>                                                                                                                                 \
  class TO_REP##Convertor                                                                                                                                                                              \
  {                                                                                                                                                                                                    \
  public:                                                                                                                                                                                              \
    TO_REP##Convertor(ConvertOrientations* filter, nx::core::DataArray<T>& input, nx::core::DataArray<K>& output)                                                                                      \
    : m_Input(input.getDataStoreRef())                                                                                                                                                                 \
    , m_Output(output.getDataStoreRef())                                                                                                                                                               \
    {                                                                                                                                                                                                  \
    }                                                                                                                                                                                                  \
    void operator()(const Range& r) const                                                                                                                                                              \
    {                                                                                                                                                                                                  \
      InputType inputInstance;                                                                                                                                                                         \
      size_t inNumComps = m_Input.getNumberOfComponents();                                                                                                                                             \
      size_t outNumComps = m_Output.getNumberOfComponents();                                                                                                                                           \
      for(size_t i = r.min(); i < r.max(); ++i)                                                                                                                                                        \
      {                                                                                                                                                                                                \
        size_t inOffset = i * inNumComps;                                                                                                                                                              \
        size_t outOffset = i * outNumComps;                                                                                                                                                            \
        for(size_t c = 0; c < inNumComps; c++)                                                                                                                                                         \
        {                                                                                                                                                                                              \
          inputInstance[c] = m_Input[inOffset + c];                                                                                                                                                    \
        }                                                                                                                                                                                              \
        OutputType outputInstance = inputInstance.to##TO_REP();                                                                                                                                        \
        for(size_t c = 0; c < outNumComps; c++)                                                                                                                                                        \
        {                                                                                                                                                                                              \
          m_Output[outOffset + c] = outputInstance[c];                                                                                                                                                 \
        }                                                                                                                                                                                              \
      }                                                                                                                                                                                                \
    }                                                                                                                                                                                                  \
                                                                                                                                                                                                       \
  private:                                                                                                                                                                                             \
    AbstractDataStore<T>& m_Input;                                                                                                                                                                     \
    AbstractDataStore<K>& m_Output;                                                                                                                                                                    \
  };

OC_TBB_IMPL(Euler)
OC_TBB_IMPL(OrientationMatrix)
OC_TBB_IMPL(Quaternion)
OC_TBB_IMPL(AxisAngle)
OC_TBB_IMPL(Rodrigues)
OC_TBB_IMPL(Homochoric)
OC_TBB_IMPL(Cubochoric)
OC_TBB_IMPL(Stereographic)

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
Result<> ConvertOrientations::operator()()
{
  using ValidateInputDataFunctionType = std::function<void(float32*)>;

  DataPath outputDataPath = m_InputValues->InputOrientationArrayPath.replaceName(m_InputValues->OutputOrientationArrayName);
  auto inputArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationArrayPath);
  auto outputArray = m_DataStructure.getDataRefAs<Float32Array>(outputDataPath);
  size_t totalPoints = inputArray.getNumberOfTuples();

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

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Euler)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(EulerConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::EulerFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::OrientationMatrix)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(OrientationMatrixConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::OrientationMatrixFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Quaternion)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(QuaternionConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::QuaternionFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::AxisAngle)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(AxisAngleConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::AxisAngleFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Rodrigues)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(RodriguesConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::RodriguesFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Homochoric)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(HomochoricConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::HomochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Cubochoric)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(CubochoricConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::CubochoricFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  if(m_InputValues->OutputType == ebsdlib::orientations::Type::Stereographic)
  {
    switch(m_InputValues->InputType)
    {
    case ebsdlib::orientations::Type::Euler:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::EulerFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::OrientationMatrix:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::OrientationMatrixFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Quaternion:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::QuaternionFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::AxisAngle:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::AxisAngleFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Rodrigues:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::RodriguesFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Homochoric:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::HomochoricFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Cubochoric:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::CubochoricFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Stereographic:
      parallelAlgorithm.execute(StereographicConvertor<float32, float32, ebsdlib::StereographicFType, ebsdlib::StereographicFType>(this, inputArray, outputArray));
      break;
    case ebsdlib::orientations::Type::Unknown:
      break;
    }
    return {};
  }

  return {};
}
