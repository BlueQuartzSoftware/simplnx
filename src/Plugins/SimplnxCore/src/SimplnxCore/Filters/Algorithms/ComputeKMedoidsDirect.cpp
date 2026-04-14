#include "ComputeKMedoidsDirect.hpp"

#include "ComputeKMedoids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <fmt/format.h>

#include <random>

using namespace nx::core;

// =============================================================================
// ComputeKMedoidsDirect — In-Core Algorithm
//
// This file implements the in-core (Direct) variant of ComputeKMedoids.
// It is selected by DispatchAlgorithm when all input arrays reside in memory.
//
// ALGORITHM OVERVIEW (Voronoi Iteration / PAM):
//   1. Randomly select k initial medoids from masked data points
//   2. Assign each point to the nearest medoid (findClusters)
//   3. For each cluster, find the member that minimizes total intra-cluster
//      distance — this becomes the new medoid (optimizeClusters)
//   4. Repeat steps 2-3 until medoids stop changing (convergence)
//
// DATA ACCESS PATTERN:
//   Uses operator[] for per-element random access to the input array, medoids
//   array, and featureIds array. This is optimal for in-memory DataStore where
//   operator[] is essentially a pointer dereference. For out-of-core data, this
//   pattern would cause chunk thrashing — see ComputeKMedoidsScanline instead.
//
// COMPLEXITY:
//   findClusters:     O(n * k * d) per iteration
//   optimizeClusters: O(k * n_i^2 * d) per iteration, where n_i is cluster size
//   Total:            O(iter * (n*k*d + k*n_i^2*d))
// =============================================================================

namespace
{
/**
 * @brief Type-specialized template that performs the actual K-Medoids computation
 * for the in-core (Direct) path.
 *
 * @tparam T The element type of the clustering array (e.g., float32, int32)
 */
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(ComputeKMedoidsDirect* filter, const IDataArray* inputIDataArray, IDataArray* medoidsIDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray,
                   usize numClusters, Int32AbstractDataStore& fIds, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_InputArray(inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Medoids(medoidsIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_Mask(maskDataArray)
  , m_NumClusters(numClusters)
  , m_FeatureIds(fIds)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  {
  }
  ~KMedoidsTemplate() = default;

  KMedoidsTemplate(const KMedoidsTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const KMedoidsTemplate&) = delete;   // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  /**
   * @brief Main K-Medoids loop: initialize medoids, then iterate findClusters +
   * optimizeClusters until convergence (medoid indices stop changing).
   */
  void operator()()
  {
    usize numTuples = m_InputArray.getNumberOfTuples();
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    std::mt19937_64 gen(m_Seed);
    std::uniform_int_distribution<usize> dist(0, numTuples - 1);

    std::vector<usize> clusterIdxs(m_NumClusters);

    usize clusterChoices = 0;
    while(clusterChoices < m_NumClusters)
    {
      usize index = dist(gen);
      if(m_Mask->isTrue(index))
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
  usize m_NumClusters = 0;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
  /**
   * @brief Assigns each data point to the nearest medoid using direct operator[] access.
   *
   * For each masked data point, computes the distance to all k medoids and assigns
   * the point to the cluster of the nearest medoid. Uses direct per-element access
   * via operator[] — optimal for in-memory data but would cause chunk thrashing for OOC.
   *
   * @param tuples Total number of tuples in the input array
   * @param dims Number of components per tuple
   */
  void findClusters(usize tuples, int32 dims)
  {
    for(usize i = 0; i < tuples; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      if(m_Mask->isTrue(i))
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
    }
  }

  // -----------------------------------------------------------------------------
  /**
   * @brief Finds the optimal medoid for each cluster by minimizing total intra-cluster distance.
   *
   * For each cluster i, iterates over all members j of that cluster. For each candidate
   * medoid j, computes the total distance from j to all other members k. The member with
   * the lowest total cost becomes the new medoid.
   *
   * Complexity: O(k * n_i^2 * dims) where n_i is the size of cluster i.
   * Uses direct per-element access via operator[].
   *
   * @param tuples Total number of tuples in the input array
   * @param dims Number of components per tuple
   * @param clusterIdxs In/out: current medoid indices, updated with new optimal medoids
   * @return Per-cluster minimum cost vector
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
        if(m_Mask->isTrue(j))
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
              if(m_FeatureIds[k] == i + 1 && m_Mask->isTrue(k))
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
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54070, message);
  }

  RunTemplateClass<KMedoidsTemplate, types::NoBooleanType>(clusteringArray->getDataType(), this, clusteringArray, m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedoidsArrayPath), maskCompare,
                                                           m_InputValues->InitClusters, m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef(),
                                                           m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
