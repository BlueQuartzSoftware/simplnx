#include "ComputeQuaternionConjugateScanline.hpp"

#include "ComputeQuaternionConjugate.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_QuaternionComponents = 4;

// A 65,536-tuple chunk keeps the reusable buffer at 1 MiB regardless of the
// total array size.
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeQuaternionConjugateScanline::ComputeQuaternionConjugateScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeQuaternionConjugateInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeQuaternionConjugateScanline::~ComputeQuaternionConjugateScanline() noexcept = default;

Result<> ComputeQuaternionConjugateScanline::operator()()
{
  const auto& input = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuaternionDataArrayPath);
  auto& output = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputDataArrayPath);

  const usize totalTuples = input.getNumberOfTuples();
  const auto& inputStore = input.getDataStoreRef();
  auto& outputStore = output.getDataStoreRef();
  auto quaternionBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_QuaternionComponents);

  for(usize offset = 0; offset < totalTuples; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(k_ChunkTuples, totalTuples - offset);
    const usize valueCount = tupleCount * k_QuaternionComponents;
    if(Result<> result = inputStore.copyIntoBuffer(offset * k_QuaternionComponents, nonstd::span<float32>(quaternionBuffer.get(), valueCount)); result.invalid())
    {
      return result;
    }

    for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
    {
      const usize valueIndex = tupleIndex * k_QuaternionComponents;
      quaternionBuffer[valueIndex] = -1.0f * quaternionBuffer[valueIndex];
      quaternionBuffer[valueIndex + 1] = -1.0f * quaternionBuffer[valueIndex + 1];
      quaternionBuffer[valueIndex + 2] = -1.0f * quaternionBuffer[valueIndex + 2];
    }

    if(Result<> result = outputStore.copyFromBuffer(offset * k_QuaternionComponents, nonstd::span<const float32>(quaternionBuffer.get(), valueCount)); result.invalid())
    {
      return result;
    }
  }

  return {};
}
