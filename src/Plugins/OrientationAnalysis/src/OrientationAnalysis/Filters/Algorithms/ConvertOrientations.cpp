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

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>

#ifndef _MSC_VER
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
#endif

using namespace nx::core;

namespace
{
// Human-readable representation names, indexed by ebsdlib::orientations::Type.
constexpr std::array<std::string_view, 8> k_TypeNames = {"Euler", "Orientation Matrix", "Quaternion", "Axis Angle", "Rodrigues", "Homochoric", "Cubochoric", "Stereographic"};

// Macro-generated workers convert 4,096-tuple local buffers through bulk I/O.
#define OC_TBB_IMPL(TO_REP)                                                                                                                                                                            \
  template <typename T, typename K, class InputType, class OutputType>                                                                                                                                 \
  class TO_REP##Convertor                                                                                                                                                                              \
  {                                                                                                                                                                                                    \
  public:                                                                                                                                                                                              \
    TO_REP##Convertor(ConvertOrientations* filter, nx::core::DataArray<T>& input, nx::core::DataArray<K>& output)                                                                                      \
    : m_Filter(filter)                                                                                                                                                                                 \
    , m_Input(input.getDataStoreRef())                                                                                                                                                                 \
    , m_Output(output.getDataStoreRef())                                                                                                                                                               \
    , m_ShouldCancel(&filter->getCancel())                                                                                                                                                             \
    {                                                                                                                                                                                                  \
    }                                                                                                                                                                                                  \
    void operator()(const Range& r) const                                                                                                                                                              \
    {                                                                                                                                                                                                  \
      static constexpr usize k_ChunkSize = 4096;                                                                                                                                                       \
      const usize inNumComps = m_Input.getNumberOfComponents();                                                                                                                                        \
      const usize outNumComps = m_Output.getNumberOfComponents();                                                                                                                                      \
      const usize totalTuples = r.max() - r.min();                                                                                                                                                     \
      const usize maxChunkTuples = std::min(k_ChunkSize, totalTuples);                                                                                                                                 \
      auto inBuffer = std::make_unique<T[]>(maxChunkTuples * inNumComps);                                                                                                                              \
      auto outBuffer = std::make_unique<K[]>(maxChunkTuples * outNumComps);                                                                                                                            \
      usize tupleIdx = r.min();                                                                                                                                                                        \
      while(tupleIdx < r.max())                                                                                                                                                                        \
      {                                                                                                                                                                                                \
        /* Poll for cancellation once per chunk (not per tuple) so a large out-of-core conversion can be interrupted. */                                                                               \
        if(m_ShouldCancel != nullptr && m_ShouldCancel->load())                                                                                                                                        \
        {                                                                                                                                                                                              \
          return;                                                                                                                                                                                      \
        }                                                                                                                                                                                              \
        const usize chunkTuples = std::min(k_ChunkSize, r.max() - tupleIdx);                                                                                                                           \
        const usize inElemCount = chunkTuples * inNumComps;                                                                                                                                            \
        const usize outElemCount = chunkTuples * outNumComps;                                                                                                                                          \
        m_Input.copyIntoBuffer(tupleIdx* inNumComps, nonstd::span<T>(inBuffer.get(), inElemCount));                                                                                                    \
        InputType inputInstance;                                                                                                                                                                       \
        for(usize t = 0; t < chunkTuples; ++t)                                                                                                                                                         \
        {                                                                                                                                                                                              \
          if(m_ShouldCancel != nullptr && m_ShouldCancel->load())                                                                                                                                      \
          {                                                                                                                                                                                            \
            return;                                                                                                                                                                                    \
          }                                                                                                                                                                                            \
          const usize inOff = t * inNumComps;                                                                                                                                                          \
          const usize outOff = t * outNumComps;                                                                                                                                                        \
          for(usize c = 0; c < inNumComps; ++c)                                                                                                                                                        \
          {                                                                                                                                                                                            \
            inputInstance[c] = inBuffer[inOff + c];                                                                                                                                                    \
          }                                                                                                                                                                                            \
          OutputType outputInstance = inputInstance.to##TO_REP();                                                                                                                                      \
          for(usize c = 0; c < outNumComps; ++c)                                                                                                                                                       \
          {                                                                                                                                                                                            \
            outBuffer[outOff + c] = outputInstance[c];                                                                                                                                                 \
          }                                                                                                                                                                                            \
        }                                                                                                                                                                                              \
        m_Output.copyFromBuffer(tupleIdx* outNumComps, nonstd::span<const K>(outBuffer.get(), outElemCount));                                                                                          \
        tupleIdx += chunkTuples;                                                                                                                                                                       \
      }                                                                                                                                                                                                \
      m_Filter->sendThreadSafeProgressMessage(r.max() - r.min());                                                                                                                                      \
    }                                                                                                                                                                                                  \
                                                                                                                                                                                                       \
  private:                                                                                                                                                                                             \
    ConvertOrientations* m_Filter = nullptr;                                                                                                                                                           \
    AbstractDataStore<T>& m_Input;                                                                                                                                                                     \
    AbstractDataStore<K>& m_Output;                                                                                                                                                                    \
    const std::atomic_bool* m_ShouldCancel = nullptr;                                                                                                                                                  \
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

ConvertOrientations::ConvertOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertOrientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ConvertOrientations::~ConvertOrientations() noexcept = default;

bool ConvertOrientations::shouldCancel() const
{
  return m_ShouldCancel;
}

void ConvertOrientations::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  const auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialPoint).count() < 1000)
  {
    return;
  }

  const auto progressInt = static_cast<usize>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_TotalPoints)) * 100.0f);
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Converting Orientations: {}% Complete", progressInt));
  m_InitialPoint = now;
}

Result<> ConvertOrientations::operator()()
{
  DataPath outputDataPath = m_InputValues->InputOrientationArrayPath.replaceName(m_InputValues->OutputOrientationArrayName);
  auto& inputArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputOrientationArrayPath);
  auto& outputArray = m_DataStructure.getDataRefAs<Float32Array>(outputDataPath);
  const usize totalPoints = inputArray.getNumberOfTuples();
  m_TotalPoints = totalPoints;

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Converting {} orientations from {} to {}", totalPoints, k_TypeNames[static_cast<usize>(m_InputValues->InputType)],
                                                             k_TypeNames[static_cast<usize>(m_InputValues->OutputType)]));

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
