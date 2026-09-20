#include <array>

#include "ComputeVectorColors.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <Eigen/Dense>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
using VectorMapType = Eigen::Map<Eigen::Vector3f>;

// Each I/O batch caps the vector and color buffers at 65,536 tuples.
constexpr usize k_ChunkTuples = 65536;
constexpr usize k_VectorComponents = 3;

/**
 * @brief Converts vector chunks to RGB chunks.
 * @tparam MaskType Specifies the mask value type.
 * @param vectors Provides three-component vector tuples.
 * @param cellVectorColors Receives three-component RGB tuples.
 * @param mask Provides optional tuple mask values.
 * @param shouldCancel Stops before the next chunk when true.
 * @return Error from bulk I/O, or success after cancellation.
 *
 * Completed chunks remain written after cancellation. Bulk I/O avoids per-tuple
 * disk access.
 */
template <typename MaskType>
Result<> ComputeVectorColorsInChunks(const AbstractDataStore<float32>& vectors, AbstractDataStore<uint8>& cellVectorColors, const AbstractDataStore<MaskType>* mask,
                                     const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = vectors.getNumberOfTuples();

  auto vectorsBuffer = std::make_unique<std::array<float32, k_ChunkTuples * k_VectorComponents>>();
  auto colorsBuffer = std::make_unique<std::array<uint8, k_ChunkTuples * k_VectorComponents>>();
  std::unique_ptr<std::array<MaskType, k_ChunkTuples>> maskBuffer;
  if(mask != nullptr)
  {
    maskBuffer = std::make_unique<std::array<MaskType, k_ChunkTuples>>();
  }

  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
    const usize valueOffset = tupleOffset * k_VectorComponents;
    const usize valueCount = tupleCount * k_VectorComponents;
    Result<> result = vectors.copyIntoBuffer(valueOffset, nonstd::span<float32>(vectorsBuffer->data(), valueCount));
    if(result.invalid())
    {
      return result;
    }

    if(mask != nullptr)
    {
      result = mask->copyIntoBuffer(tupleOffset, nonstd::span<MaskType>(maskBuffer->data(), tupleCount));
      if(result.invalid())
      {
        return result;
      }
    }

    for(usize i = 0; i < tupleCount; i++)
    {
      const usize index = i * k_VectorComponents;
      (*colorsBuffer)[index] = 0;
      (*colorsBuffer)[index + 1] = 0;
      (*colorsBuffer)[index + 2] = 0;

      if(mask == nullptr || static_cast<bool>((*maskBuffer)[i]))
      {
        std::array<float32, 3> dir = {0.0f, 0.0f, 0.0f};
        dir[0] = (*vectorsBuffer)[index];
        dir[1] = (*vectorsBuffer)[index + 1];
        dir[2] = (*vectorsBuffer)[index + 2];
        VectorMapType array(dir.data());
        array.normalize();

        if(dir[2] < 0)
        {
          // Fold antipodal directions into the upper hemisphere before coloring.
          array = array * -1.0f;
        }

        float32 trend = std::atan2(array[1], array[0]) * (Constants::k_RadToDegF);
        float32 plunge = std::acos(array[2]) * (Constants::k_RadToDegF);
        if(trend < 0.0f)
        {
          trend += 360.0f;
        }

        // Trend selects the RGB hue. Plunge blends that hue toward white.
        float32 r = 0, g = 0, b = 0;
        if(trend <= 120.0f)
        {
          r = 255.0f * ((120.0f - trend) / 120.0f);
          g = 255.0f * (trend / 120.0f);
          b = 0.0f;
        }
        if(trend > 120.0f && trend <= 240.0f)
        {
          trend -= 120.0f;
          r = 0.0f;
          g = 255.0f * ((120.0f - trend) / 120.0f);
          b = 255.0f * (trend / 120.0f);
        }
        if(trend > 240.0f && trend < 360.0f)
        {
          trend -= 240.0f;
          r = 255.0f * (trend / 120.0f);
          g = 0.0f;
          b = 255.0f * ((120.0f - trend) / 120.0f);
        }
        float32 deltaR = 255.0f - r;
        float32 deltaG = 255.0f - g;
        float32 deltaB = 255.0f - b;
        r += (deltaR * ((90.0f - plunge) / 90.0f));
        g += (deltaG * ((90.0f - plunge) / 90.0f));
        b += (deltaB * ((90.0f - plunge) / 90.0f));
        if(r > 255.0f)
        {
          r = 255.0f;
        }
        if(g > 255.0f)
        {
          g = 255.0f;
        }
        if(b > 255.0f)
        {
          b = 255.0f;
        }

        Rgb argb = RgbColor::dRgb(static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b), 255);
        (*colorsBuffer)[index] = RgbColor::dRed(argb);
        (*colorsBuffer)[index + 1] = RgbColor::dGreen(argb);
        (*colorsBuffer)[index + 2] = RgbColor::dBlue(argb);
      }
    }

    result = cellVectorColors.copyFromBuffer(valueOffset, nonstd::span<const uint8>(colorsBuffer->data(), valueCount));
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
} // namespace

ComputeVectorColors::ComputeVectorColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeVectorColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeVectorColors::~ComputeVectorColors() noexcept = default;

const std::atomic_bool& ComputeVectorColors::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeVectorColors::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  const auto& vectors = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VectorsArrayPath).getDataStoreRef();
  auto& cellVectorColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellVectorColorsArrayPath).getDataStoreRef();

  if(!m_InputValues->UseMask)
  {
    return ComputeVectorColorsInChunks<uint8>(vectors, cellVectorColors, nullptr, m_ShouldCancel);
  }

  const auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  if(maskArray != nullptr)
  {
    switch(maskArray->getDataType())
    {
    case DataType::boolean: {
      const auto& mask = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
      return ComputeVectorColorsInChunks(vectors, cellVectorColors, &mask, m_ShouldCancel);
    }
    case DataType::uint8: {
      const auto& mask = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
      return ComputeVectorColorsInChunks(vectors, cellVectorColors, &mask, m_ShouldCancel);
    }
    default:
      break;
    }
  }

  // Filter validation rejects this state. Direct algorithm callers can still reach it.
  return MakeErrorResult(-54700, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
}
