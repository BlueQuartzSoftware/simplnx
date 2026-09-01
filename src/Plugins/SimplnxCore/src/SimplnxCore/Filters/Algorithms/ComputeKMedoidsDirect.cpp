#include "ComputeKMedoidsDirect.hpp"

#include "ComputeKMedoids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <fmt/format.h>

#include <random>

using namespace nx::core;

namespace
{
/**
 * @class KMedoidsTemplate
 * @brief Performs typed Voronoi iterations with direct element access.
 * @tparam T Input and medoid value type.
 */
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(ComputeKMedoidsDirect* filter, const IDataArray* inputIDataArray, IDataArray* medoidsIDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray,
                   bool useMask, usize numClusters, Int32AbstractDataStore& fIds, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_InputArray(inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Medoids(medoidsIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Mask(maskDataArray)
  , m_UseMask(useMask)
  , m_NumClusters(numClusters)
  , m_FeatureIds(fIds)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  {
  }
  ~KMedoidsTemplate() = default;

  KMedoidsTemplate(const KMedoidsTemplate&) = delete;
  void operator=(const KMedoidsTemplate&) = delete;

  // -----------------------------------------------------------------------------
  /**
   * @brief Initializes medoids and repeats assignment and optimization phases.
   *
   * Seeded selection samples with replacement, so duplicate medoids are valid.
   * Exact medoid-index equality controls convergence. There is no iteration
   * limit. Cancellation can leave partial assignments from the active phase.
   */
  void operator()()
  {
    usize numTuples = m_InputArray.getNumberOfTuples();
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    if(m_Filter->getCancel())
    {
      return;
    }
    for(int32 component = 0; component < numCompDims; component++)
    {
      m_Medoids[component] = T{};
    }

    std::mt19937_64 gen(m_Seed);
    std::uniform_int_distribution<usize> dist(0, numTuples - 1);

    std::vector<usize> clusterIdxs(m_NumClusters);

    usize clusterChoices = 0;
    while(clusterChoices < m_NumClusters)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      usize index = dist(gen);
      if(!m_UseMask || m_Mask->isTrue(index))
      {
        clusterIdxs[clusterChoices] = index;
        clusterChoices++;
      }
    }

    for(usize i = 0; i < m_NumClusters; i++)
    {
      for(int32 j = 0; j < numCompDims; j++)
      {
        m_Medoids[numCompDims * (i + 1) + j] = m_InputArray[numCompDims * clusterIdxs[i] + j];
      }
    }

    findClusters(numTuples, numCompDims);

    std::vector<usize> optClusterIdxs(clusterIdxs);

    std::vector<float64> costs = optimizeClusters(numTuples, numCompDims, clusterIdxs);

    bool update = optClusterIdxs == clusterIdxs ? false : true;
    usize iteration = 1;

    while(update)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      findClusters(numTuples, numCompDims);

      optClusterIdxs = clusterIdxs;

      costs = optimizeClusters(numTuples, numCompDims, clusterIdxs);

      update = optClusterIdxs == clusterIdxs ? false : true;

      float64 sum = std::accumulate(std::begin(costs), std::end(costs), 0.0);
      m_Filter->updateProgress(fmt::format("Clustering Data || Iteration {} || Total Cost: {}", iteration, sum));
      iteration++;
    }
  }

private:
  using DataArrayT = DataArray<T>;
  using AbstractDataStoreT = AbstractDataStore<T>;
  ComputeKMedoidsDirect* m_Filter;
  const AbstractDataStoreT& m_InputArray;
  AbstractDataStoreT& m_Medoids;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  bool m_UseMask = false;
  usize m_NumClusters = 0;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
  /**
   * @brief Assigns each selected tuple to its nearest medoid.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   *
   * Masked tuples receive cluster ID zero. Cancellation leaves earlier
   * assignments in FeatureIds.
   */
  void findClusters(usize tuples, int32 dims)
  {
    for(usize i = 0; i < tuples; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        float64 minDist = std::numeric_limits<float64>::max();
        for(int32 j = 0; j < m_NumClusters; j++)
        {
          float64 dist = ClusterUtilities::GetDistance(m_InputArray, (dims * i), m_Medoids, (dims * (j + 1)), dims, m_DistMetric);
          if(dist < minDist)
          {
            minDist = dist;
            m_FeatureIds[i] = j + 1;
          }
        }
      }
      else
      {
        m_FeatureIds[i] = 0;
      }
    }
  }

  // -----------------------------------------------------------------------------
  /**
   * @brief Finds each cluster member with the lowest total peer distance.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   * @param clusterIdxs Supplies current medoids and receives optimized indices.
   * @return Minimum cost for each cluster, or an empty vector after cancellation.
   *
   * Strict comparison keeps the first minimum-cost member in tuple order. An
   * empty cluster retains its prior medoid. The direct path uses quadratic
   * member comparisons and can thrash when forced onto out-of-core storage.
   */
  std::vector<float64> optimizeClusters(usize tuples, int32 dims, std::vector<usize>& clusterIdxs)
  {
    std::vector<float64> minCosts(m_NumClusters, std::numeric_limits<float64>::max());

    for(usize i = 0; i < m_NumClusters; i++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      for(usize j = 0; j < tuples; j++)
      {
        if(m_Filter->getCancel())
        {
          return {};
        }
        if(!m_UseMask || m_Mask->isTrue(j))
        {
          if(m_FeatureIds[j] == i + 1)
          {
            float64 cost = 0.0;
            for(usize k = 0; k < tuples; k++)
            {
              if(m_Filter->getCancel())
              {
                return {};
              }
              if(m_FeatureIds[k] == i + 1 && (!m_UseMask || m_Mask->isTrue(k)))
              {
                cost += ClusterUtilities::GetDistance(m_InputArray, (dims * k), m_InputArray, (dims * j), dims, m_DistMetric);
              }
            }

            if(cost < minCosts[i])
            {
              minCosts[i] = cost;
              clusterIdxs[i] = j;
            }
          }
        }
      }
    }

    for(usize i = 0; i < m_NumClusters; i++)
    {
      for(int32 j = 0; j < dims; j++)
      {
        m_Medoids[dims * (i + 1) + j] = m_InputArray[dims * clusterIdxs[i] + j];
      }
    }

    return minCosts;
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeKMedoidsDirect::ComputeKMedoidsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKMedoidsDirect::~ComputeKMedoidsDirect() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMedoidsDirect::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMedoidsDirect::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeKMedoidsDirect::operator()()
{
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  if(clusteringArray->getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-54071, "Compute K Medoids requires at least one input tuple.");
  }
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range&)
    {
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-54070, message);
    }
    bool hasEligible = false;
    constexpr usize k_EligibilityChunk = 4096;
    if(auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath); maskArray->getDataType() == DataType::boolean)
    {
      std::array<bool, k_EligibilityChunk> values = {};
      const auto& store = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
      for(usize offset = 0; offset < clusteringArray->getNumberOfTuples(); offset += k_EligibilityChunk)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize count = std::min(k_EligibilityChunk, clusteringArray->getNumberOfTuples() - offset);
        auto result = store.copyIntoBuffer(offset, nonstd::span<bool>(values.data(), count));
        if(result.invalid())
        {
          return result;
        }
        hasEligible = hasEligible || std::any_of(values.cbegin(), values.cbegin() + count, [](bool value) { return value; });
      }
    }
    else if(maskArray->getDataType() == DataType::uint8)
    {
      std::array<uint8, k_EligibilityChunk> values = {};
      const auto& store = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
      for(usize offset = 0; offset < clusteringArray->getNumberOfTuples(); offset += k_EligibilityChunk)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize count = std::min(k_EligibilityChunk, clusteringArray->getNumberOfTuples() - offset);
        auto result = store.copyIntoBuffer(offset, nonstd::span<uint8>(values.data(), count));
        if(result.invalid())
        {
          return result;
        }
        hasEligible = hasEligible || std::any_of(values.cbegin(), values.cbegin() + count, [](uint8 value) { return value != 0; });
      }
    }
    if(!hasEligible)
    {
      return MakeErrorResult(-54072, "Compute K Medoids found no eligible tuples in the mask.");
    }
  }

  RunTemplateClass<KMedoidsTemplate, types::NoBooleanType>(
      clusteringArray->getDataType(), this, clusteringArray, m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedoidsArrayPath), maskCompare, m_InputValues->UseMask, m_InputValues->InitClusters,
      m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef(), m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
