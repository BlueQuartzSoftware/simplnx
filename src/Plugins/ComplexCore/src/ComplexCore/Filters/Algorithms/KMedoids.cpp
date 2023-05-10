#include "KMedoids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <random>

using namespace complex;

namespace
{
/**
 * @brief The DistanceTemplate class contains a templated function getDistance to find the distance, via a variety of
 * metrics, between two vectors of arbitrary dimensions. The developer should ensure that the pointers passed to
 * getDistance do indeed contain vectors of the same component dimensions and start at the desired tuples.
 */
template <typename leftDataType, typename rightDataType>
auto GetDistance(const leftDataType& leftVector, usize leftOffset, const rightDataType& rightVector, usize rightOffset, usize compDims, int distMetric)
{
  float64 dist = 0.0;
  float64 lVal = 0.0;
  float64 rVal = 0.0;

  float64 epsilon = std::numeric_limits<float64>::min();

  // Euclidean
  if(distMetric == 0)
  {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += (lVal - rVal) * (lVal - rVal);
    }

    dist = std::sqrt(dist);
  }
  // Squared Euclidean
  else if(distMetric == 1)
  {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += (lVal - rVal) * (lVal - rVal);
    }
  }
  // Manhattan
  else if(distMetric == 2)
  {
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      dist += std::abs(lVal - rVal);
    }
  }
  // Cosine
  else if(distMetric == 3)
  {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += lVal * rVal;
      x += lVal * lVal;
      y += rVal * rVal;
    }
    dist = 1 - (r / (sqrt(x * y) + epsilon));
  }
  // Pearson
  else if(distMetric == 4)
  {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    float64 xAvg = 0;
    float64 yAvg = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      xAvg += lVal;
      yAvg += rVal;
    }
    xAvg /= static_cast<float64>(compDims);
    yAvg /= static_cast<float64>(compDims);
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += (lVal - xAvg) * (rVal - yAvg);
      x += (lVal - xAvg) * (lVal - xAvg);
      y += (rVal - yAvg) * (rVal - yAvg);
    }
    dist = 1 - (r / (sqrt(x * y) + epsilon));
  }
  // Squared Pearson
  else if(distMetric == 5)
  {
    float64 r = 0;
    float64 x = 0;
    float64 y = 0;
    float64 xAvg = 0;
    float64 yAvg = 0;
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      xAvg += lVal;
      yAvg += rVal;
    }
    xAvg /= static_cast<float64>(compDims);
    yAvg /= static_cast<float64>(compDims);
    for(usize i = 0; i < compDims; i++)
    {
      lVal = static_cast<float64>(leftVector[i + leftOffset]);
      rVal = static_cast<float64>(rightVector[i + rightOffset]);
      r += (lVal - xAvg) * (rVal - yAvg);
      x += (lVal - xAvg) * (lVal - xAvg);
      y += (rVal - yAvg) * (rVal - yAvg);
    }
    dist = 1 - ((r * r) / ((x * y) + epsilon));
  }

  // Return the correct primitive type for distance
  return dist;
}

template <typename T>
class KMedoidsTemplate
{
public:
  using Self = KMedoidsTemplate;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  KMedoidsTemplate(KMedoids* filter, const IDataArray& inputIDataArray, IDataArray& medoidsIDataArray, const BoolArray& maskDataArray, usize numClusters, Int32Array& fIds, int32 distMetric,
                   std::mt19937_64::result_type seed)
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
  virtual ~KMedoidsTemplate() = default;

  KMedoidsTemplate(const KMedoidsTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const KMedoidsTemplate&) = delete;   // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputArray.getNumberOfTuples();
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    usize rangeMin = 0;
    usize rangeMax = numTuples - 1;
    std::mt19937_64 gen(m_Seed);
    std::uniform_int_distribution<usize> dist(rangeMin, rangeMax);

    std::vector<usize> clusterIdxs(m_NumClusters);
    usize clusterChoices = 0;

    while(clusterChoices < m_NumClusters)
    {
      usize index = dist(gen);
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
    std::vector<float64> costs;

    costs = optimizeClusters(numTuples, numCompDims, clusterIdxs);

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
  int32 m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
  void findClusters(usize tuples, int32 dims)
  {
    float64 dist = 0.0;

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
          dist = GetDistance(m_InputArray, (dims * i), m_Medoids, (dims * (j + 1)), dims, m_DistMetric);
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
    float64 dist = 0.0;
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
          float64 cost = 0.0;
          if(m_FeatureIds[j] == i + 1)
          {
            for(usize k = 0; k < tuples; k++)
            {
              if(m_Filter->getCancel())
              {
                return {};
              }
              if(m_FeatureIds[k] == i + 1 && m_Mask[k])
              {
                dist = GetDistance(m_InputArray, (dims * k), m_InputArray, (dims * j), dims, m_DistMetric);
                cost += dist;
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
