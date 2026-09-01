#include "ComputeKMeansScanline.hpp"

#include "ComputeKMeans.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkValues = 65536;
constexpr usize k_MaskChunkTuples = 65536;

/**
 * @class ChunkMaskReader
 * @brief Presents an optional Bool/UInt8 mask through one reusable bounded tuple page.
 *
 * A null mask reports every tuple as selected. Page reads avoid single-value
 * out-of-core access during centroid selection.
 */
class ChunkMaskReader
{
public:
  explicit ChunkMaskReader(const IDataArray* mask)
  : m_Mask(mask)
  {
    if(m_Mask != nullptr)
    {
      m_BoolBuffer = std::make_unique<bool[]>(k_MaskChunkTuples);
      m_UInt8Buffer = std::make_unique<uint8[]>(k_MaskChunkTuples);
    }
  }

  /**
   * @brief Loads one mask page.
   * @param offset First tuple in the page.
   * @param count Number of tuples to load.
   * @return Success, or a bulk-read error.
   */
  Result<> load(usize offset, usize count)
  {
    m_Offset = offset;
    m_Count = count;
    if(m_Mask == nullptr)
    {
      return {};
    }
    if(m_Mask->getDataType() == DataType::boolean)
    {
      return m_Mask->getIDataStoreRefAs<AbstractDataStore<bool>>().copyIntoBuffer(offset, nonstd::span<bool>(m_BoolBuffer.get(), count));
    }
    return m_Mask->getIDataStoreRefAs<AbstractDataStore<uint8>>().copyIntoBuffer(offset, nonstd::span<uint8>(m_UInt8Buffer.get(), count));
  }

  bool value(usize index) const
  {
    if(m_Mask == nullptr)
    {
      return true;
    }
    const usize localIndex = index - m_Offset;
    return m_Mask->getDataType() == DataType::boolean ? m_BoolBuffer[localIndex] : m_UInt8Buffer[localIndex] != 0;
  }

  /**
   * @brief Reads one mask value and loads its page when necessary.
   * @param index Tuple index to read.
   * @param tupleCount Total mask tuple count.
   * @return The mask value, or a bulk-read error.
   */
  Result<bool> valueAt(usize index, usize tupleCount)
  {
    if(m_Mask == nullptr)
    {
      return {true};
    }
    if(index < m_Offset || index >= m_Offset + m_Count)
    {
      const usize pageStart = (index / k_MaskChunkTuples) * k_MaskChunkTuples;
      auto result = load(pageStart, std::min(k_MaskChunkTuples, tupleCount - pageStart));
      if(result.invalid())
      {
        return ConvertInvalidResult<bool>(std::move(result));
      }
    }
    return {value(index)};
  }

  /**
   * @brief Tests whether the mask selects at least one tuple.
   * @param tupleCount Total input tuple count.
   * @return True when one selected tuple exists, or a bulk-read error.
   * @note This validation scan does not inspect the cancellation flag.
   */
  Result<bool> hasTrueValue(usize tupleCount)
  {
    if(m_Mask == nullptr)
    {
      return {tupleCount != 0};
    }
    for(usize offset = 0; offset < tupleCount; offset += k_MaskChunkTuples)
    {
      const usize count = std::min(k_MaskChunkTuples, tupleCount - offset);
      auto result = load(offset, count);
      if(result.invalid())
      {
        return ConvertInvalidResult<bool>(std::move(result));
      }
      for(usize index = 0; index < count; index++)
      {
        if(value(index + offset))
        {
          return {true};
        }
      }
    }
    return {false};
  }

private:
  const IDataArray* m_Mask = nullptr;
  std::unique_ptr<bool[]> m_BoolBuffer;
  std::unique_ptr<uint8[]> m_UInt8Buffer;
  usize m_Offset = 0;
  usize m_Count = 0;
};

/**
 * @class ComputeKMeansTemplate
 * @brief Typed Lloyd iteration using chunked assignments and cluster-scale centroid state.
 * @tparam T Input and centroid value type.
 *
 * Centroids remain resident because they scale with K and component count. Cell
 * inputs, masks, and feature IDs use fixed pages. One pass accumulates all
 * centroid components.
 */
template <typename T>
class ComputeKMeansTemplate
{
public:
  ComputeKMeansTemplate(ComputeKMeansScanline& filter, const IDataArray& inputArray, IDataArray& meansArray, const IDataArray* maskArray, Int32AbstractDataStore& featureIds,
                        const ComputeKMeansInputValues& inputValues)
  : m_Filter(filter)
  , m_Input(inputArray.getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Means(meansArray.getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Mask(maskArray)
  , m_FeatureIds(featureIds)
  , m_InputValues(inputValues)
  {
  }

  /**
   * @brief Selects initial centroids and repeats assignment and mean phases.
   * @return Success, or a shape, overflow, cluster-ID, or bulk-transfer error.
   *
   * Sampling permits duplicate centroids. For multi-tuple input, the legacy
   * index formula excludes the final tuple. The convergence test reads flat
   * means indices 1 through K and does not inspect all components.
   *
   * Cancellation returns success. Output pages from earlier phases remain.
   */
  Result<> operator()()
  {
    const usize tupleCount = m_Input.getNumberOfTuples();
    const usize components = m_Input.getNumberOfComponents();
    const usize clusters = m_InputValues.InitClusters;
    if(tupleCount == 0 || components == 0 || clusters == 0)
    {
      return MakeErrorResult(-54061, "Compute K Means requires at least one tuple, component, and cluster.");
    }
    if(clusters > std::numeric_limits<usize>::max() / components - 1)
    {
      return MakeErrorResult(-54062, "Compute K Means centroid storage dimensions overflow the platform index type.");
    }

    ChunkMaskReader maskReader(m_Mask);
    auto anyMaskResult = maskReader.hasTrueValue(tupleCount);
    if(anyMaskResult.invalid())
    {
      return ConvertResult(std::move(anyMaskResult));
    }
    if(!anyMaskResult.value())
    {
      return MakeErrorResult(-54063, "Compute K Means cannot initialize clusters because the mask contains no selected tuples.");
    }

    std::mt19937_64 generator(m_InputValues.Seed);
    std::uniform_real_distribution<float64> distribution(0.0, 1.0);
    std::vector<usize> centroidIndices(clusters);
    usize selected = 0;
    const usize rangeMax = tupleCount - 1;
    while(selected < clusters)
    {
      if(m_Filter.getCancel())
      {
        return {};
      }
      const usize index = std::floor(distribution(generator) * static_cast<float64>(rangeMax));
      auto maskResult = maskReader.valueAt(index, tupleCount);
      if(maskResult.invalid())
      {
        return ConvertResult(std::move(maskResult));
      }
      if(maskResult.value())
      {
        centroidIndices[selected++] = index;
      }
    }

    auto tupleBuffer = std::make_unique<T[]>(components);
    for(usize cluster = 0; cluster < clusters; cluster++)
    {
      auto result = m_Input.copyIntoBuffer(centroidIndices[cluster] * components, nonstd::span<T>(tupleBuffer.get(), components));
      if(result.invalid())
      {
        return result;
      }
      result = m_Means.copyFromBuffer((cluster + 1) * components, nonstd::span<const T>(tupleBuffer.get(), components));
      if(result.invalid())
      {
        return result;
      }
    }

    const usize meansSize = (clusters + 1) * components;
    std::vector<T> meansSnapshot(meansSize);
    std::vector<float64> oldMeans(clusters);
    std::vector<float64> differences(clusters);
    usize updateCheck = 0;
    usize iteration = 1;
    while(updateCheck != clusters)
    {
      if(m_Filter.getCancel())
      {
        return {};
      }
      auto result = findClusters(tupleCount, components, maskReader);
      if(result.invalid())
      {
        return result;
      }
      if(m_Filter.getCancel())
      {
        return {};
      }
      result = m_Means.copyIntoBuffer(0, nonstd::span<T>(meansSnapshot.data(), meansSize));
      if(result.invalid())
      {
        return result;
      }
      for(usize cluster = 0; cluster < clusters; cluster++)
      {
        oldMeans[cluster] = static_cast<float64>(meansSnapshot[cluster + 1]);
      }
      result = findMeans(tupleCount, components);
      if(result.invalid())
      {
        return result;
      }
      if(m_Filter.getCancel())
      {
        return {};
      }
      result = m_Means.copyIntoBuffer(0, nonstd::span<T>(meansSnapshot.data(), meansSize));
      if(result.invalid())
      {
        return result;
      }
      updateCheck = 0;
      for(usize cluster = 0; cluster < clusters; cluster++)
      {
        differences[cluster] = oldMeans[cluster] - static_cast<float64>(meansSnapshot[cluster + 1]);
        if(std::numeric_limits<float64>::epsilon() > std::fabs(differences[cluster]))
        {
          updateCheck++;
        }
      }
      m_Filter.updateProgress(fmt::format("Clustering Data || Iteration {} || Total Mean Shift: {}", iteration++, std::accumulate(differences.cbegin(), differences.cend(), 0.0)));
    }
    return {};
  }

private:
  /**
   * @brief Assigns selected tuples to their nearest cached centroids.
   * @param tupleCount Number of input tuples.
   * @param components Number of components in each tuple.
   * @param maskReader Supplies aligned mask pages.
   * @return Success, or a bulk-transfer error.
   *
   * Existing assignment pages are read first so masked tuples keep their prior
   * IDs. Cancellation leaves completed pages in the output store.
   */
  Result<> findClusters(usize tupleCount, usize components, ChunkMaskReader& maskReader)
  {
    const usize meansSize = (m_InputValues.InitClusters + 1) * components;
    std::vector<T> means(meansSize);
    auto result = m_Means.copyIntoBuffer(0, nonstd::span<T>(means.data(), meansSize));
    if(result.invalid())
    {
      return result;
    }
    const usize tuplesPerChunk = std::max<usize>(1, k_ChunkValues / components);
    auto input = std::make_unique<T[]>(tuplesPerChunk * components);
    auto featureIds = std::make_unique<int32[]>(tuplesPerChunk);
    for(usize offset = 0; offset < tupleCount; offset += tuplesPerChunk)
    {
      if(m_Filter.getCancel())
      {
        return {};
      }
      const usize count = std::min(tuplesPerChunk, tupleCount - offset);
      result = m_Input.copyIntoBuffer(offset * components, nonstd::span<T>(input.get(), count * components));
      if(result.invalid())
        return result;
      result = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIds.get(), count));
      if(result.invalid())
        return result;
      result = maskReader.load(offset, count);
      if(result.invalid())
        return result;
      for(usize local = 0; local < count; local++)
      {
        if(maskReader.value(offset + local))
        {
          float64 minimum = std::numeric_limits<float64>::max();
          for(usize cluster = 0; cluster < m_InputValues.InitClusters; cluster++)
          {
            const float64 distance = ClusterUtilities::GetDistance(input.get(), local * components, means, (cluster + 1) * components, components, m_InputValues.DistanceMetric);
            if(distance < minimum)
            {
              minimum = distance;
              featureIds[local] = static_cast<int32>(cluster + 1);
            }
          }
        }
      }
      result = m_FeatureIds.copyFromBuffer(offset, nonstd::span<const int32>(featureIds.get(), count));
      if(result.invalid())
        return result;
    }
    return {};
  }

  /**
   * @brief Recomputes all arithmetic means with one chunked input pass.
   * @param tupleCount Number of input tuples.
   * @param components Number of components in each tuple.
   * @return Success, or a cluster-ID or bulk-transfer error.
   *
   * All tuples contribute to their current assignment. Masked tuples normally
   * remain in reserved bucket zero. Cancellation does not publish partial sums.
   */
  Result<> findMeans(usize tupleCount, usize components)
  {
    const usize meansSize = (m_InputValues.InitClusters + 1) * components;
    std::vector<T> sums(meansSize, static_cast<T>(0));
    std::vector<usize> counts(m_InputValues.InitClusters + 1, 0);
    const usize tuplesPerChunk = std::max<usize>(1, k_ChunkValues / components);
    auto input = std::make_unique<T[]>(tuplesPerChunk * components);
    auto featureIds = std::make_unique<int32[]>(tuplesPerChunk);
    for(usize offset = 0; offset < tupleCount; offset += tuplesPerChunk)
    {
      if(m_Filter.getCancel())
        return {};
      const usize count = std::min(tuplesPerChunk, tupleCount - offset);
      auto result = m_Input.copyIntoBuffer(offset * components, nonstd::span<T>(input.get(), count * components));
      if(result.invalid())
        return result;
      result = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIds.get(), count));
      if(result.invalid())
        return result;
      for(usize local = 0; local < count; local++)
      {
        const usize feature = static_cast<usize>(featureIds[local]);
        if(feature > m_InputValues.InitClusters)
        {
          return MakeErrorResult(-54064, "Compute K Means encountered a cluster id outside the configured cluster range.");
        }
        for(usize component = 0; component < components; component++)
          sums[feature * components + component] += input[local * components + component];
        counts[feature]++;
      }
    }
    for(usize feature = 0; feature <= m_InputValues.InitClusters; feature++)
    {
      for(usize component = 0; component < components; component++)
      {
        T& value = sums[feature * components + component];
        value = counts[feature] == 0 ? static_cast<T>(0) : value / static_cast<T>(static_cast<float64>(counts[feature]));
      }
    }
    return m_Means.copyFromBuffer(0, nonstd::span<const T>(sums.data(), meansSize));
  }

  ComputeKMeansScanline& m_Filter;
  const AbstractDataStore<T>& m_Input;
  AbstractDataStore<T>& m_Means;
  const IDataArray* m_Mask = nullptr;
  Int32AbstractDataStore& m_FeatureIds;
  const ComputeKMeansInputValues& m_InputValues;
};

/**
 * @struct ExecuteKMeansFunctor
 * @brief Dispatches the runtime numeric type to the scanline implementation.
 */
struct ExecuteKMeansFunctor
{
  /**
   * @brief Constructs and executes one typed K-Means implementation.
   * @tparam T Input and centroid value type.
   * @param filter Supplies messaging and cancellation.
   * @param input Supplies input tuples.
   * @param means Receives cluster centroids.
   * @param mask Supplies an optional Bool or UInt8 mask.
   * @param featureIds Receives cluster assignments.
   * @param inputValues Supplies immutable settings.
   * @return Result from the typed implementation.
   */
  template <typename T>
  Result<> operator()(ComputeKMeansScanline& filter, const IDataArray& input, IDataArray& means, const IDataArray* mask, Int32AbstractDataStore& featureIds,
                      const ComputeKMeansInputValues& inputValues) const
  {
    return ComputeKMeansTemplate<T>(filter, input, means, mask, featureIds, inputValues)();
  }
};
} // namespace

ComputeKMeansScanline::ComputeKMeansScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             const ComputeKMeansInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeKMeansScanline::~ComputeKMeansScanline() noexcept = default;

void ComputeKMeansScanline::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

const std::atomic_bool& ComputeKMeansScanline::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeKMeansScanline::operator()()
{
  const auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  const IDataArray* maskArray = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;
  auto& meansArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MeansArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  return ExecuteDataFunctionNoBool(ExecuteKMeansFunctor{}, clusteringArray.getDataType(), *this, clusteringArray, meansArray, maskArray, featureIds, *m_InputValues);
}
