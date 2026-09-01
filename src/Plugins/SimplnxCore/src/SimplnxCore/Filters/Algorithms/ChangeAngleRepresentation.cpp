#include "ChangeAngleRepresentation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkValues = 65536;

namespace EulerAngleConversionType
{
constexpr uint64 DegreesToRadians = 0;
constexpr uint64 RadiansToDegrees = 1;
} // namespace EulerAngleConversionType

float32 GetConversionFactor(uint64 conversionType)
{
  if(conversionType == EulerAngleConversionType::DegreesToRadians)
  {
    return static_cast<float32>(numbers::pi / 180.0f);
  }
  if(conversionType == EulerAngleConversionType::RadiansToDegrees)
  {
    return static_cast<float32>(180.0f / numbers::pi);
  }
  return 1.0f;
}

/**
 * @brief Converts one data store in sequential bounded pages.
 * @param angles Stores the in-place Float32 values.
 * @param conversionFactor Multiplier for each value.
 * @param shouldCancel Signals cancellation between pages.
 * @return Source or destination bulk-I/O errors.
 *
 * Cancellation returns success after pages that are already written.
 */
Result<> ConvertValuesInChunks(Float32AbstractDataStore& angles, float32 conversionFactor, const std::atomic_bool& shouldCancel)
{
  const usize totalValues = angles.getSize();
  if(totalValues == 0 || shouldCancel)
  {
    return {};
  }

  auto buffer = std::make_unique<float32[]>(k_ChunkValues);
  for(usize offset = 0; offset < totalValues; offset += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkValues, totalValues - offset);
    Result<> result = angles.copyIntoBuffer(offset, nonstd::span<float32>(buffer.get(), count));
    if(result.invalid())
    {
      return result;
    }

    for(usize index = 0; index < count; index++)
    {
      buffer[index] *= conversionFactor;
    }

    result = angles.copyFromBuffer(offset, nonstd::span<const float32>(buffer.get(), count));
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

/**
 * @class ConvertContiguousValuesImpl
 * @brief Multiplies one disjoint range in a stable contiguous allocation.
 *
 * Each worker checks cancellation before its range, not during the range.
 */
class ConvertContiguousValuesImpl
{
public:
  ConvertContiguousValuesImpl(float32* values, float32 conversionFactor, const std::atomic_bool& shouldCancel)
  : m_Values(values)
  , m_ConversionFactor(conversionFactor)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~ConvertContiguousValuesImpl() noexcept = default;

  void operator()(const Range& range) const
  {
    if(m_ShouldCancel)
    {
      return;
    }

    for(usize index = range.min(); index < range.max(); index++)
    {
      m_Values[index] *= m_ConversionFactor;
    }
  }

private:
  float32* m_Values = nullptr;
  float32 m_ConversionFactor = 1.0f;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ChangeAngleRepresentationDirect
 * @brief Uses direct parallel access for a concrete Float32DataStore.
 *
 * Other store implementations delegate to the bounded page function.
 */
class ChangeAngleRepresentationDirect
{
public:
  ChangeAngleRepresentationDirect(Float32Array& angles, float32 conversionFactor, const std::atomic_bool& shouldCancel)
  : m_Angles(angles)
  , m_ConversionFactor(conversionFactor)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> operator()() const
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    auto& anglesStore = m_Angles.getDataStoreRef();
    auto* inMemoryStore = dynamic_cast<Float32DataStore*>(&anglesStore);
    if(inMemoryStore == nullptr)
    {
      return ConvertValuesInChunks(anglesStore, m_ConversionFactor, m_ShouldCancel);
    }

    // Disjoint worker ranges operate directly on the stable contiguous backing allocation.
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, inMemoryStore->getSize());
    parallelAlgorithm.execute(ConvertContiguousValuesImpl(inMemoryStore->data(), m_ConversionFactor, m_ShouldCancel));
    return {};
  }

private:
  Float32Array& m_Angles;
  float32 m_ConversionFactor = 1.0f;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ChangeAngleRepresentationScanline
 * @brief Uses sequential bounded pages for storage-neutral conversion.
 */
class ChangeAngleRepresentationScanline
{
public:
  ChangeAngleRepresentationScanline(Float32Array& angles, float32 conversionFactor, const std::atomic_bool& shouldCancel)
  : m_Angles(angles)
  , m_ConversionFactor(conversionFactor)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> operator()() const
  {
    return ConvertValuesInChunks(m_Angles.getDataStoreRef(), m_ConversionFactor, m_ShouldCancel);
  }

private:
  Float32Array& m_Angles;
  float32 m_ConversionFactor = 1.0f;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

ChangeAngleRepresentation::ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ChangeAngleRepresentationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ChangeAngleRepresentation::~ChangeAngleRepresentation() noexcept = default;

Result<> ChangeAngleRepresentation::operator()()
{
  auto& angles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AnglesArrayPath);
  const float32 conversionFactor = GetConversionFactor(m_InputValues->ConversionTypeIndex);
  return DispatchAlgorithm<ChangeAngleRepresentationDirect, ChangeAngleRepresentationScanline>({&angles}, angles, conversionFactor, m_ShouldCancel);
}
