#include "SilhouetteScanline.hpp"

#include "Silhouette.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace nx::core;

namespace
{
// Each source, mask, Feature ID, and output tile contains at most 128 tuples.
constexpr usize k_TileTuples = 128;

/**
 * @brief Reads an optional mask into a uniform byte tile.
 * @param dataStructure Contains the optional mask.
 * @param inputValues Selects mask use and path.
 * @param offset First mask tuple.
 * @param count Number of mask tuples.
 * @param buffer Receives zero or nonzero byte values.
 * @param boolBuffer Supplies contiguous temporary storage for Bool masks.
 * @return Mask bulk-read or unsupported-type result.
 * @pre buffer and boolBuffer contain at least count values.
 *
 * A requested range avoids a tuple-count synthetic mask and gives the pairwise
 * loop one representation for Bool and UInt8 values.
 */
Result<> ReadMask(DataStructure& dataStructure, const SilhouetteInputValues& inputValues, usize offset, usize count, std::vector<uint8>& buffer, bool* boolBuffer)
{
  if(!inputValues.UseMask)
  {
    std::fill_n(buffer.data(), count, 1);
    return {};
  }

  const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(inputValues.MaskArrayPath);
  if(maskArray.getDataType() == DataType::boolean)
  {
    auto result = dataStructure.getDataRefAs<BoolArray>(inputValues.MaskArrayPath).getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolBuffer, count));
    if(result.invalid())
    {
      return result;
    }
    for(usize i = 0; i < count; i++)
    {
      buffer[i] = boolBuffer[i] ? 1 : 0;
    }
    return result;
  }
  if(maskArray.getDataType() == DataType::uint8)
  {
    return dataStructure.getDataRefAs<UInt8Array>(inputValues.MaskArrayPath).getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(buffer.data(), count));
  }
  return MakeErrorResult(-54080, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues.MaskArrayPath.toString()));
}

/**
 * @brief Computes exact silhouette scores with bounded tuple tiles.
 * @tparam T Specifies the clustering-array value type.
 * @param dataStructure Contains participating arrays.
 * @param shouldCancel Signals cancellation between passes and tiles.
 * @param inputValues Selects metric, mask, and paths.
 * @return Bulk-I/O, mask-type, Feature ID, or size-overflow result.
 * @pre Participating arrays have equal tuple counts.
 *
 * Feature discovery permits sparse IDs. Counting supplies exact mean
 * denominators. The pair pass retains one outer tile's cluster accumulators.
 * Cancellation returns success and does not roll back completed output tiles.
 */
template <typename T>
Result<> ExecuteScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const SilhouetteInputValues& inputValues)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputValues.ClusteringArrayPath);
  const auto& inputStore = inputArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& featureStore = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureIdsArrayPath).getDataStoreRef();
  auto& outputStore = dataStructure.getDataRefAs<Float64Array>(inputValues.SilhouetteArrayPath).getDataStoreRef();
  const usize tupleCount = featureStore.getNumberOfTuples();
  const usize componentCount = inputStore.getNumberOfComponents();

  // Map sparse positive Feature IDs to dense accumulator columns.
  std::unordered_map<int32, usize> denseFeatureIds;
  std::vector<int32> featureBuffer(k_TileTuples);
  std::vector<uint8> maskBuffer(k_TileTuples, 1);
  std::unique_ptr<bool[]> boolMaskBuffer = std::make_unique<bool[]>(k_TileTuples);
  for(usize offset = 0; offset < tupleCount; offset += k_TileTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_TileTuples, tupleCount - offset);
    auto result = featureStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.data(), count));
    if(result.invalid())
    {
      return result;
    }
    result = ReadMask(dataStructure, inputValues, offset, count, maskBuffer, boolMaskBuffer.get());
    if(result.invalid())
    {
      return result;
    }
    for(usize i = 0; i < count; i++)
    {
      if(featureBuffer[i] < 0)
      {
        return MakeErrorResult(-54081, fmt::format("Feature ID {} at tuple {} is negative", featureBuffer[i], offset + i));
      }
      if(featureBuffer[i] > 0 && !denseFeatureIds.contains(featureBuffer[i]))
      {
        denseFeatureIds.insert({featureBuffer[i], denseFeatureIds.size() + 1});
      }
    }
  }

  const usize clusterCount = denseFeatureIds.size();
  if(clusterCount == std::numeric_limits<usize>::max() || componentCount > std::numeric_limits<usize>::max() / k_TileTuples || clusterCount + 1 > std::numeric_limits<usize>::max() / k_TileTuples)
  {
    return MakeErrorResult(-54083, "Silhouette tile buffer size overflows the platform usize limit");
  }

  // Count enabled tuples so each cluster sum has one mean denominator.
  std::vector<float64> featureCounts(clusterCount + 1, 0.0);
  for(usize offset = 0; offset < tupleCount; offset += k_TileTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_TileTuples, tupleCount - offset);
    auto result = featureStore.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.data(), count));
    if(result.invalid())
    {
      return result;
    }
    result = ReadMask(dataStructure, inputValues, offset, count, maskBuffer, boolMaskBuffer.get());
    if(result.invalid())
    {
      return result;
    }
    for(usize i = 0; i < count; i++)
    {
      if(maskBuffer[i] != 0)
      {
        const usize cluster = featureBuffer[i] == 0 ? 0 : denseFeatureIds.at(featureBuffer[i]);
        featureCounts[cluster]++;
      }
    }
  }

  // Compare each outer tile with all inner tiles, then publish its scores.
  std::vector<T> outerValues(k_TileTuples * componentCount);
  std::vector<T> innerValues(k_TileTuples * componentCount);
  std::vector<int32> innerFeatures(k_TileTuples);
  std::vector<uint8> innerMask(k_TileTuples, 1);
  std::vector<float64> outputBuffer(k_TileTuples, 0.0);
  std::vector<float64> clusterDistances(k_TileTuples * (clusterCount + 1), 0.0);
  for(usize outerOffset = 0; outerOffset < tupleCount; outerOffset += k_TileTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize outerCount = std::min(k_TileTuples, tupleCount - outerOffset);
    auto result = inputStore.copyIntoBuffer(outerOffset * componentCount, nonstd::span<T>(outerValues.data(), outerCount * componentCount));
    if(result.invalid())
    {
      return result;
    }
    result = featureStore.copyIntoBuffer(outerOffset, nonstd::span<int32>(featureBuffer.data(), outerCount));
    if(result.invalid())
    {
      return result;
    }
    result = ReadMask(dataStructure, inputValues, outerOffset, outerCount, maskBuffer, boolMaskBuffer.get());
    if(result.invalid())
    {
      return result;
    }

    std::fill(clusterDistances.begin(), clusterDistances.end(), 0.0);
    for(usize innerOffset = 0; innerOffset < tupleCount; innerOffset += k_TileTuples)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize innerCount = std::min(k_TileTuples, tupleCount - innerOffset);
      result = inputStore.copyIntoBuffer(innerOffset * componentCount, nonstd::span<T>(innerValues.data(), innerCount * componentCount));
      if(result.invalid())
      {
        return result;
      }
      result = featureStore.copyIntoBuffer(innerOffset, nonstd::span<int32>(innerFeatures.data(), innerCount));
      if(result.invalid())
      {
        return result;
      }
      result = ReadMask(dataStructure, inputValues, innerOffset, innerCount, innerMask, boolMaskBuffer.get());
      if(result.invalid())
      {
        return result;
      }

      for(usize outerIndex = 0; outerIndex < outerCount; outerIndex++)
      {
        if(maskBuffer[outerIndex] == 0)
        {
          continue;
        }
        for(usize innerIndex = 0; innerIndex < innerCount; innerIndex++)
        {
          if(innerMask[innerIndex] == 0)
          {
            continue;
          }
          const usize cluster = innerFeatures[innerIndex] == 0 ? 0 : denseFeatureIds.at(innerFeatures[innerIndex]);
          clusterDistances[outerIndex * (clusterCount + 1) + cluster] +=
              ClusterUtilities::GetDistance(outerValues, outerIndex * componentCount, innerValues, innerIndex * componentCount, componentCount, inputValues.DistanceMetric);
        }
      }
    }

    for(usize outerIndex = 0; outerIndex < outerCount; outerIndex++)
    {
      if(maskBuffer[outerIndex] == 0)
      {
        outputBuffer[outerIndex] = 0.0;
        continue;
      }
      const usize ownCluster = featureBuffer[outerIndex] == 0 ? 0 : denseFeatureIds.at(featureBuffer[outerIndex]);
      float64 inClusterDistance = clusterDistances[outerIndex * (clusterCount + 1) + ownCluster];
      if(ownCluster > 0)
      {
        inClusterDistance /= featureCounts[ownCluster];
      }
      float64 outClusterDistance = 0.0;
      float64 minimumDistance = std::numeric_limits<float64>::max();
      for(usize cluster = 1; cluster <= clusterCount; cluster++)
      {
        if(cluster == ownCluster || featureCounts[cluster] == 0.0)
        {
          continue;
        }
        const float64 distance = clusterDistances[outerIndex * (clusterCount + 1) + cluster] / featureCounts[cluster];
        if(distance < minimumDistance)
        {
          minimumDistance = distance;
          outClusterDistance = distance;
        }
      }
      outputBuffer[outerIndex] = (outClusterDistance - inClusterDistance) / std::max(outClusterDistance, inClusterDistance);
    }
    result = outputStore.copyFromBuffer(outerOffset, nonstd::span<const float64>(outputBuffer.data(), outerCount));
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

/**
 * @class SilhouetteScanlineRunner
 * @brief Adapts runtime numeric dispatch to ExecuteScanline().
 * @tparam T Specifies the clustering-array value type.
 *
 * RunTemplateClass requires a class-template callable. This adapter borrows the
 * execution context and does not extend any lifetime.
 */
template <typename T>
class SilhouetteScanlineRunner
{
public:
  /**
   * @brief Initializes one typed dispatch adapter.
   * @param dataStructure Contains participating arrays.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects metric, mask, and paths.
   * @param result Receives the typed calculation result.
   * @pre All arguments outlive this adapter.
   */
  SilhouetteScanlineRunner(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const SilhouetteInputValues* inputValues, Result<>& result)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  , m_Result(result)
  {
  }

  /**
   * @brief Runs the selected numeric specialization and stores its result.
   */
  void operator()()
  {
    m_Result = ExecuteScanline<T>(m_DataStructure, m_ShouldCancel, *m_InputValues);
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const SilhouetteInputValues* m_InputValues = nullptr;
  Result<>& m_Result;
};
} // namespace

SilhouetteScanline::SilhouetteScanline(DataStructure& dataStructure, const IFilter::MessageHandler&, const std::atomic_bool& shouldCancel, const SilhouetteInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

SilhouetteScanline::~SilhouetteScanline() noexcept = default;

Result<> SilhouetteScanline::operator()()
{
  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  Result<> result;
  RunTemplateClass<SilhouetteScanlineRunner, types::NoBooleanType>(inputArray.getDataType(), m_DataStructure, m_ShouldCancel, m_InputValues, result);
  return result;
}
