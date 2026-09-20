#include "ComputeKMedoidsScanline.hpp"

#include "ComputeKMedoids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <numeric>
#include <random>

using namespace nx::core;

namespace
{
constexpr usize k_TileTuples = 4096;

/**
 * @class MaskReader
 * @brief Normalizes optional Bool or UInt8 mask tiles to bytes.
 *
 * A disabled mask fills the caller buffer with selected values. This avoids a
 * synthetic full-size mask.
 */
class MaskReader
{
public:
  MaskReader(const IDataArray* array, bool useMask)
  : m_Array(array)
  , m_UseMask(useMask)
  {
  }

  /**
   * @brief Reads one mask tile.
   * @param offset First tuple in the tile.
   * @param count Number of tuples to read.
   * @param values Receives normalized byte values.
   * @return Success, or a type or bulk-read error.
   */
  Result<> read(usize offset, usize count, nonstd::span<uint8> values) const
  {
    if(!m_UseMask)
    {
      std::fill(values.begin(), values.begin() + count, uint8{1});
      return {};
    }
    if(m_Array->getDataType() == DataType::boolean)
    {
      auto result = m_Array->getIDataStoreRefAs<AbstractDataStore<bool>>().copyIntoBuffer(offset, nonstd::span<bool>(m_BoolBuffer.data(), count));
      if(result.invalid())
      {
        return result;
      }
      for(usize i = 0; i < count; i++)
      {
        values[i] = m_BoolBuffer[i] ? 1 : 0;
      }
      return {};
    }
    if(m_Array->getDataType() == DataType::uint8)
    {
      return m_Array->getIDataStoreRefAs<AbstractDataStore<uint8>>().copyIntoBuffer(offset, values.subspan(0, count));
    }
    return MakeErrorResult(-54070, "Mask array must have Bool or UInt8 data type.");
  }

private:
  const IDataArray* m_Array = nullptr;
  bool m_UseMask = false;
  mutable std::array<bool, k_TileTuples> m_BoolBuffer = {};
};

/**
 * @class KMedoidsTemplate
 * @brief Performs typed K-Medoids iteration with bounded tiles.
 * @tparam T Input and medoid value type.
 *
 * Cluster medoids and costs scale with K, while membership, distance, and mask
 * data use fixed tiles. Candidate costs retain tuple order and strict
 * comparisons, so ties match the direct path.
 */
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(ComputeKMedoidsScanline* filter, const IDataArray* inputArray, IDataArray* medoidsArray, const IDataArray* maskArray, bool useMask, usize clusters,
                   Int32AbstractDataStore& featureIds, ClusterUtilities::DistanceMetric metric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_Input(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Medoids(medoidsArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Mask(maskArray, useMask)
  , m_Clusters(clusters)
  , m_FeatureIds(featureIds)
  , m_Metric(metric)
  , m_Seed(seed)
  {
  }

  /**
   * @brief Initializes medoids and repeats assignment and optimization phases.
   * @return Success, or a mask, shape, overflow, recovery, or transfer error.
   *
   * Seeded selection samples with replacement. Exact medoid-index equality
   * controls convergence, and there is no iteration limit. Cancellation returns
   * success and preserves output from completed bounded operations.
   */
  Result<> operator()()
  {
    const usize tuples = m_Input.getNumberOfTuples();
    const usize dims = m_Input.getNumberOfComponents();
    if(tuples == 0)
    {
      return MakeErrorResult(-54071, "Compute K Medoids requires at least one input tuple.");
    }
    if(m_Clusters > static_cast<usize>(std::numeric_limits<int32>::max()) || m_Clusters == std::numeric_limits<usize>::max() || dims == 0 ||
       (m_Clusters + 1) > std::numeric_limits<usize>::max() / dims || k_TileTuples > std::numeric_limits<usize>::max() / dims)
    {
      return MakeErrorResult(-54073, "Compute K Medoids input dimensions or cluster count overflow the supported address range.");
    }

    std::vector<usize> medoidIndices(m_Clusters);
    auto initializationResult = initialize(tuples, dims, medoidIndices);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }
    if(m_Filter->getCancel())
    {
      return {};
    }

    auto assignmentResult = findClusters(tuples, dims);
    if(assignmentResult.invalid())
    {
      return assignmentResult;
    }
    if(m_Filter->getCancel())
    {
      return {};
    }
    std::vector<usize> priorIndices = medoidIndices;
    std::vector<float64> costs;
    auto optimizeResult = optimizeClusters(tuples, dims, medoidIndices, costs);
    if(optimizeResult.invalid())
    {
      return optimizeResult;
    }
    if(m_Filter->getCancel())
    {
      return {};
    }

    for(usize iteration = 1; priorIndices != medoidIndices; iteration++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      auto assignResult = findClusters(tuples, dims);
      if(assignResult.invalid())
      {
        return assignResult;
      }
      if(m_Filter->getCancel())
      {
        return {};
      }
      priorIndices = medoidIndices;
      auto updateResult = optimizeClusters(tuples, dims, medoidIndices, costs);
      if(updateResult.invalid())
      {
        return updateResult;
      }
      if(m_Filter->getCancel())
      {
        return {};
      }
      m_Filter->updateProgress(fmt::format("Clustering Data || Iteration {} || Total Cost: {}", iteration, std::accumulate(costs.cbegin(), costs.cend(), 0.0)));
    }
    return {};
  }

private:
  /**
   * @brief Selects eligible medoid indices and writes their tuples.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   * @param medoidIndices Receives the sampled tuple indices.
   * @return Success, or a mask, medoid-recovery, or transfer error.
   *
   * The first pass proves that at least one tuple is eligible. Sampling then
   * uses replacement and reuses the current mask tile.
   */
  Result<> initialize(usize tuples, usize dims, std::vector<usize>& medoidIndices)
  {
    std::vector<uint8> maskBuffer(k_TileTuples);
    bool hasEligible = false;
    for(usize offset = 0; offset < tuples; offset += k_TileTuples)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      const usize count = std::min(k_TileTuples, tuples - offset);
      auto maskResult = m_Mask.read(offset, count, nonstd::span<uint8>(maskBuffer.data(), maskBuffer.size()));
      if(maskResult.invalid())
      {
        return maskResult;
      }
      hasEligible = hasEligible || std::any_of(maskBuffer.cbegin(), maskBuffer.cbegin() + count, [](uint8 value) { return value != 0; });
    }
    if(!hasEligible)
    {
      return MakeErrorResult(-54072, "Compute K Medoids found no eligible tuples in the mask.");
    }
    std::mt19937_64 generator(m_Seed);
    std::uniform_int_distribution<usize> distribution(0, tuples - 1);
    usize cachedOffset = std::numeric_limits<usize>::max();
    usize cachedCount = 0;
    for(usize choice = 0; choice < m_Clusters;)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      const usize index = distribution(generator);
      const usize offset = (index / k_TileTuples) * k_TileTuples;
      if(offset != cachedOffset)
      {
        cachedOffset = offset;
        cachedCount = std::min(k_TileTuples, tuples - offset);
        auto maskResult = m_Mask.read(offset, cachedCount, nonstd::span<uint8>(maskBuffer.data(), maskBuffer.size()));
        if(maskResult.invalid())
        {
          return maskResult;
        }
      }
      if(maskBuffer[index - cachedOffset] != 0)
      {
        medoidIndices[choice++] = index;
      }
    }
    return writeMedoids(tuples, dims, medoidIndices);
  }

  /**
   * @brief Assigns each selected tuple to its nearest cached medoid.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   * @return Success, or a bulk-transfer error.
   *
   * Masked tuples receive cluster ID zero. Cancellation leaves completed
   * assignment tiles in FeatureIds.
   */
  Result<> findClusters(usize tuples, usize dims)
  {
    std::vector<T> medoids((m_Clusters + 1) * dims);
    auto medoidResult = m_Medoids.copyIntoBuffer(0, nonstd::span<T>(medoids.data(), medoids.size()));
    if(medoidResult.invalid())
    {
      return medoidResult;
    }
    std::vector<T> input(k_TileTuples * dims);
    std::vector<int32> ids(k_TileTuples);
    std::vector<uint8> mask(k_TileTuples);
    for(usize offset = 0; offset < tuples; offset += k_TileTuples)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      const usize count = std::min(k_TileTuples, tuples - offset);
      auto inputResult = m_Input.copyIntoBuffer(offset * dims, nonstd::span<T>(input.data(), count * dims));
      if(inputResult.invalid())
      {
        return inputResult;
      }
      auto idsResult = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(ids.data(), count));
      if(idsResult.invalid())
      {
        return idsResult;
      }
      auto maskResult = m_Mask.read(offset, count, nonstd::span<uint8>(mask.data(), mask.size()));
      if(maskResult.invalid())
      {
        return maskResult;
      }
      for(usize local = 0; local < count; local++)
      {
        if(mask[local] == 0)
        {
          ids[local] = 0;
          continue;
        }
        float64 minimum = std::numeric_limits<float64>::max();
        for(usize cluster = 0; cluster < m_Clusters; cluster++)
        {
          const float64 distance = ClusterUtilities::GetDistance(input, local * dims, medoids, (cluster + 1) * dims, dims, m_Metric);
          if(distance < minimum)
          {
            minimum = distance;
            ids[local] = static_cast<int32>(cluster + 1);
          }
        }
      }
      auto writeResult = m_FeatureIds.copyFromBuffer(offset, nonstd::span<const int32>(ids.data(), count));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    return {};
  }

  /**
   * @brief Finds each cluster member with the lowest total peer distance.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   * @param medoidIndices Supplies current medoids and receives optimized indices.
   * @param costs Receives the minimum cost for each cluster.
   * @return Success, or a medoid-recovery or bulk-transfer error.
   *
   * Candidate and target tiles provide bounded RAM but retain quadratic input
   * reads. Strict comparison keeps the first minimum-cost tuple. Cancellation
   * does not publish the active medoid update.
   */
  Result<> optimizeClusters(usize tuples, usize dims, std::vector<usize>& medoidIndices, std::vector<float64>& costs)
  {
    costs.assign(m_Clusters, std::numeric_limits<float64>::max());
    std::vector<T> candidate(k_TileTuples * dims), inner(k_TileTuples * dims);
    std::vector<int32> candidateIds(k_TileTuples), innerIds(k_TileTuples);
    std::vector<uint8> candidateMask(k_TileTuples), innerMask(k_TileTuples);
    std::vector<float64> candidateCosts(k_TileTuples);
    for(usize candidateOffset = 0; candidateOffset < tuples; candidateOffset += k_TileTuples)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      const usize candidateCount = std::min(k_TileTuples, tuples - candidateOffset);
      auto result = readTile(candidateOffset, candidateCount, dims, candidate, candidateIds, candidateMask);
      if(result.invalid())
      {
        return result;
      }
      std::fill(candidateCosts.begin(), candidateCosts.begin() + candidateCount, 0.0);
      for(usize innerOffset = 0; innerOffset < tuples; innerOffset += k_TileTuples)
      {
        if(m_Filter->getCancel())
        {
          return {};
        }
        const usize innerCount = std::min(k_TileTuples, tuples - innerOffset);
        result = readTile(innerOffset, innerCount, dims, inner, innerIds, innerMask);
        if(result.invalid())
        {
          return result;
        }
        for(usize candidateLocal = 0; candidateLocal < candidateCount; candidateLocal++)
        {
          if(candidateMask[candidateLocal] == 0 || candidateIds[candidateLocal] <= 0 || candidateIds[candidateLocal] > static_cast<int32>(m_Clusters))
          {
            continue;
          }
          for(usize innerLocal = 0; innerLocal < innerCount; innerLocal++)
          {
            if(innerMask[innerLocal] != 0 && innerIds[innerLocal] == candidateIds[candidateLocal])
            {
              candidateCosts[candidateLocal] += ClusterUtilities::GetDistance(candidate, candidateLocal * dims, inner, innerLocal * dims, dims, m_Metric);
            }
          }
        }
      }
      for(usize candidateLocal = 0; candidateLocal < candidateCount; candidateLocal++)
      {
        if(candidateMask[candidateLocal] != 0 && candidateIds[candidateLocal] > 0 && candidateIds[candidateLocal] <= static_cast<int32>(m_Clusters))
        {
          const usize cluster = static_cast<usize>(candidateIds[candidateLocal] - 1);
          if(candidateCosts[candidateLocal] < costs[cluster])
          {
            costs[cluster] = candidateCosts[candidateLocal];
            medoidIndices[cluster] = candidateOffset + candidateLocal;
          }
        }
      }
    }
    return writeMedoids(tuples, dims, medoidIndices);
  }

  /**
   * @brief Reads aligned input, assignment, and mask tiles.
   * @param offset First tuple in the tile.
   * @param count Number of tuples to read.
   * @param dims Number of components in each tuple.
   * @param values Receives input values.
   * @param ids Receives cluster assignments.
   * @param mask Receives normalized mask values.
   * @return Success, or a bulk-transfer error.
   */
  Result<> readTile(usize offset, usize count, usize dims, std::vector<T>& values, std::vector<int32>& ids, std::vector<uint8>& mask)
  {
    auto result = m_Input.copyIntoBuffer(offset * dims, nonstd::span<T>(values.data(), count * dims));
    if(result.invalid())
    {
      return result;
    }
    result = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(ids.data(), count));
    if(result.invalid())
    {
      return result;
    }
    return m_Mask.read(offset, count, nonstd::span<uint8>(mask.data(), mask.size()));
  }

  /**
   * @brief Gathers medoid tuples and writes the cluster-scale output.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   * @param medoidIndices Identifies the source tuple for each cluster.
   * @return Success, or a medoid-recovery or bulk-transfer error.
   *
   * The output write occurs only after all requested tuples are recovered.
   */
  Result<> writeMedoids(usize tuples, usize dims, const std::vector<usize>& medoidIndices)
  {
    std::vector<T> input(k_TileTuples * dims), medoids((m_Clusters + 1) * dims);
    std::vector<uint8> found(m_Clusters, 0);
    for(usize offset = 0; offset < tuples; offset += k_TileTuples)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      const usize count = std::min(k_TileTuples, tuples - offset);
      auto result = m_Input.copyIntoBuffer(offset * dims, nonstd::span<T>(input.data(), count * dims));
      if(result.invalid())
      {
        return result;
      }
      for(usize cluster = 0; cluster < m_Clusters; cluster++)
      {
        if(!found[cluster] && medoidIndices[cluster] >= offset && medoidIndices[cluster] < offset + count)
        {
          std::copy_n(input.data() + (medoidIndices[cluster] - offset) * dims, dims, medoids.data() + (cluster + 1) * dims);
          found[cluster] = 1;
        }
      }
    }
    if(std::any_of(found.cbegin(), found.cend(), [](uint8 value) { return value == 0; }))
    {
      return MakeErrorResult(-54074, "Compute K Medoids could not recover one or more selected medoids from the input array.");
    }
    return m_Medoids.copyFromBuffer(0, nonstd::span<const T>(medoids.data(), medoids.size()));
  }

  ComputeKMedoidsScanline* m_Filter = nullptr;
  const AbstractDataStore<T>& m_Input;
  AbstractDataStore<T>& m_Medoids;
  MaskReader m_Mask;
  usize m_Clusters = 0;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_Metric;
  std::mt19937_64::result_type m_Seed;
};
} // namespace

ComputeKMedoidsScanline::ComputeKMedoidsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                 const KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ComputeKMedoidsScanline::~ComputeKMedoidsScanline() noexcept = default;

void ComputeKMedoidsScanline::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

const std::atomic_bool& ComputeKMedoidsScanline::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeKMedoidsScanline::operator()()
{
  auto* clustering = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto* medoids = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedoidsArrayPath);
  auto* mask = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;
  if(m_InputValues->UseMask && (mask == nullptr || (mask->getDataType() != DataType::boolean && mask->getDataType() != DataType::uint8)))
  {
    return MakeErrorResult(-54070, "Mask array must exist and have Bool or UInt8 data type.");
  }
  return RunTemplateClass<KMedoidsTemplate, types::NoBooleanType>(clustering->getDataType(), this, clustering, medoids, mask, m_InputValues->UseMask, m_InputValues->InitClusters,
                                                                  m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef(), m_InputValues->DistanceMetric,
                                                                  m_InputValues->Seed);
}
