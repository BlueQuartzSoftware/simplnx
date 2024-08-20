#include "ComputeKMedoids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <random>

using namespace nx::core;

namespace
{
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(ComputeKMedoids* filter, const IDataArray* inputIDataArray, IDataArray* medoidsIDataArray, const std::unique_ptr<MaskCompare>& maskDataArray, usize numClusters,
                   Int32AbstractDataStore& fIds, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
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
  ComputeKMedoids* m_Filter;
  const AbstractDataStoreT& m_InputArray;
  AbstractDataStoreT& m_Medoids;
  const std::unique_ptr<MaskCompare>& m_Mask;
  usize m_NumClusters;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
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
ComputeKMedoids::ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKMedoids::~ComputeKMedoids() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMedoids::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMedoids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeKMedoids::operator()()
{
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54070, message);
  }
  RunTemplateClass<KMedoidsTemplate, types::NoBooleanType>(clusteringArray->getDataType(), this, clusteringArray, m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedoidsArrayPath), maskCompare,
                                                           m_InputValues->InitClusters, m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef(),
                                                           m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
