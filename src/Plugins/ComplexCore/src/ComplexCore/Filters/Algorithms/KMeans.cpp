#include "KMeans.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/KUtilities.hpp"

#include <RandLib/distributions/BasicRandGenerator.hpp>
#include <RandLib/distributions/UniformRand.hpp>

using namespace complex;

namespace
{
template <typename T>
class KMeansTemplate
{
public:
  KMeansTemplate(KMeans* filter, const IDataArray& inputIDataArray, IDataArray& meansIDataArray, const BoolArray& maskDataArray, usize numClusters, Int32Array& fIds,
                 KUtilities::DistanceMetric distMetric, uint64 seed)
  : m_Filter(filter)
  , m_InputArray(dynamic_cast<const DataArrayT&>(inputIDataArray))
  , m_Means(dynamic_cast<DataArrayT&>(meansIDataArray))
  , m_Mask(maskDataArray)
  , m_NumClusters(numClusters)
  , m_FeatureIds(fIds)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  {
  }
  ~KMeansTemplate() = default;

  KMeansTemplate(const KMeansTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const KMeansTemplate&) = delete; // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputArray.getNumberOfTuples();
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    const usize rangeMax = numTuples - 1;

    RandLib::UniformRand<float32, RandLib::JKissRandEngine> uDist = RandLib::UniformRand<float32, RandLib::JKissRandEngine>(0.0f, 1.0f);
    uDist.Reseed(m_Seed);

    std::vector<usize> clusterIdxs(m_NumClusters);

    std::vector<float32> data(m_NumClusters);
    uDist.Sample(data);

    usize clusterChoices = 0;
    for(float32 num : data)
    {
      usize index = std::floor(num * static_cast<float64>(rangeMax));
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
        m_Means[numCompDims * (i + 1) + j] = m_InputArray[numCompDims * clusterIdxs[i] + j];
      }
    }

    std::vector<float64> oldMeans(m_NumClusters);
    std::vector<float64> differences(m_NumClusters);
    usize iteration = 1;
    usize updateCheck = 0;
    while(updateCheck != m_NumClusters)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      findClusters(numTuples, numCompDims);

      for(usize i = 0; i < m_NumClusters; i++)
      {
        oldMeans[i] = m_Means[i + 1];
      }

      findMeans(numTuples, numCompDims);

      updateCheck = 0;
      for(usize i = 0; i < m_NumClusters; i++)
      {
        differences[i] = oldMeans[i] - m_Means[i + 1];
        if(closeEnough<float64>(differences[i], 0.0))
        {
          updateCheck++;
        }
      }

      float64 sum = std::accumulate(std::begin(differences), std::end(differences), 0.0);
      m_Filter->updateProgress(fmt::format("Clustering Data || Iteration {} || Total Mean Shift: {}", iteration, sum));
      iteration++;
    }
  }

private:
  using DataArrayT = DataArray<T>;
  KMeans* m_Filter;
  const DataArrayT& m_InputArray;
  DataArrayT& m_Means;
  const BoolArray& m_Mask;
  usize m_NumClusters;
  Int32Array& m_FeatureIds;
  KUtilities::DistanceMetric m_DistMetric;
  uint64 m_Seed;

  // -----------------------------------------------------------------------------
  template <typename K>
  bool closeEnough(const K& a, const K& b, const K& epsilon = std::numeric_limits<K>::epsilon())
  {
    return (epsilon > fabs(a - b));
  }

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
          float64 dist = KUtilities::GetDistance(m_InputArray, (dims * i), m_Means, (dims * (j + 1)), dims, m_DistMetric);
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
  void findMeans(usize tuples, int32 dims)
  {
    std::vector<usize> counts(m_NumClusters + 1, 0);

    for(usize i = 0; i <= m_NumClusters; i++)
    {
      for(usize j = 0; j < dims; j++)
      {
        m_Means[dims * i + j] = 0.0;
      }
    }

    for(usize i = 0; i < dims; i++)
    {
      for(usize j = 0; j < tuples; j++)
      {
        int32 feature = m_FeatureIds[j];
        m_Means[dims * feature + i] += static_cast<float64>(m_InputArray[dims * j + i]);
        counts[feature] += 1;
      }
      for(usize j = 0; j <= m_NumClusters; j++)
      {
        if(counts[j] == 0)
        {
          m_Means[dims * j + i] = 0.0;
        }
        else
        {
          m_Means[dims * j + i] /= static_cast<float64>(counts[j]);
        }
      }
      std::fill(std::begin(counts), std::end(counts), 0);
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
KMeans::KMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMeansInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
KMeans::~KMeans() noexcept = default;

// -----------------------------------------------------------------------------
void KMeans::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& KMeans::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> KMeans::operator()()
{
  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  RunTemplateClass<KMeansTemplate, types::NoBooleanType>(clusteringArray.getDataType(), this, clusteringArray, m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MeansArrayPath),
                                                         m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath), m_InputValues->InitClusters,
                                                         m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath), m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
