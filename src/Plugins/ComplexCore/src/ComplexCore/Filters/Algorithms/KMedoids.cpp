#include "KMedoids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/KUtilities.hpp"

#include <RandLib/distributions/BasicRandGenerator.hpp>
#include <RandLib/distributions/UniformDiscreteRand.hpp>

using namespace complex;

namespace
{
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(KMedoids* filter, const IDataArray& inputIDataArray, IDataArray& medoidsIDataArray, const BoolArray& maskDataArray, usize numClusters, Int32Array& fIds,
                   KUtilities::DistanceMetric distMetric, uint64 seed)
  : m_Filter(filter)
  , m_InputArray(dynamic_cast<const DataArrayT&>(inputIDataArray))
  , m_Medoids(dynamic_cast<DataArrayT&>(medoidsIDataArray))
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
    int64 numTuples = static_cast<int64>(m_InputArray.getNumberOfTuples());
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    RandLib::UniformDiscreteRand<int64, RandLib::JLKiss64RandEngine> uDist = RandLib::UniformDiscreteRand<int64, RandLib::JLKiss64RandEngine>(0, numTuples - 1);
    uDist.Reseed(m_Seed);

    std::vector<usize> clusterIdxs(m_NumClusters);

    usize clusterChoices = 0;
    std::vector<int64> data(m_NumClusters);
    uDist.Sample(data);

    for(int64 index : data)
    {
      if(m_Mask[index])
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
  KMedoids* m_Filter;
  const DataArrayT& m_InputArray;
  DataArrayT& m_Medoids;
  const BoolArray& m_Mask;
  usize m_NumClusters;
  Int32Array& m_FeatureIds;
  KUtilities::DistanceMetric m_DistMetric;
  uint64 m_Seed;

  // -----------------------------------------------------------------------------
  void findClusters(usize tuples, int32 dims)
  {
    for(usize i = 0; i < tuples; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      if(m_Mask[i])
      {
        float64 minDist = std::numeric_limits<float64>::max();
        for(int32 j = 0; j < m_NumClusters; j++)
        {
          float64 dist = KUtilities::GetDistance(m_InputArray, (dims * i), m_Medoids, (dims * (j + 1)), dims, m_DistMetric);
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
        if(m_Mask[j])
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
              if(m_FeatureIds[k] == i + 1 && m_Mask[k])
              {
                cost += KUtilities::GetDistance(m_InputArray, (dims * k), m_InputArray, (dims * j), dims, m_DistMetric);
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
KMedoids::KMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
KMedoids::~KMedoids() noexcept = default;

// -----------------------------------------------------------------------------
void KMedoids::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& KMedoids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> KMedoids::operator()()
{
  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  RunTemplateClass<KMedoidsTemplate, types::NoBooleanType>(clusteringArray.getDataType(), this, clusteringArray, m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MedoidsArrayPath),
                                                           m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath), m_InputValues->InitClusters,
                                                           m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath), m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
