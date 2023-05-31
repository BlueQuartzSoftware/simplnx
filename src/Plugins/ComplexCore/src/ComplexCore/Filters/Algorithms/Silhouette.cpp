#include "Silhouette.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/KUtilities.hpp"

#include <unordered_set>

using namespace complex;

namespace
{
template <typename T>
class SilhouetteTemplate
{
public:
  using Self = SilhouetteTemplate;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;

  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  SilhouetteTemplate(const IDataArray& inputIDataArray, Float64Array& outputDataArray, const BoolArray& maskDataArray, usize numClusters, const Int32Array& featureIds,
                     KUtilities::DistanceMetric distMetric)
  : m_InputData(dynamic_cast<const DataArrayT&>(inputIDataArray))
  , m_OutputData(outputDataArray)
  , m_Mask(maskDataArray)
  , m_NumClusters(numClusters)
  , m_FeatureIds(featureIds)
  , m_DistMetric(distMetric)
  {
  }
  ~SilhouetteTemplate() = default;

public:
  SilhouetteTemplate(const SilhouetteTemplate&) = delete;            // Copy Constructor Not Implemented
  SilhouetteTemplate(SilhouetteTemplate&&) = delete;                 // Move Constructor Not Implemented
  SilhouetteTemplate& operator=(const SilhouetteTemplate&) = delete; // Copy Assignment Not Implemented
  SilhouetteTemplate& operator=(SilhouetteTemplate&&) = delete;      // Move Assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputData.getNumberOfTuples();
    usize numCompDims = m_InputData.getNumberOfComponents();
    usize totalClusters = m_NumClusters + 1;
    std::vector<float64> inClusterDist(numTuples, 0.0);
    std::vector<float64> outClusterMinDist(numTuples, 0.0);
    std::vector<float64> numTuplesPerFeature(totalClusters, 0.0);
    std::vector<std::vector<float64>> clusterDist(numTuples, std::vector<float64>(totalClusters, 0.0));

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Mask[i])
      {
        numTuplesPerFeature[m_FeatureIds[i]]++;
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Mask[i])
      {
        for(usize j = 0; j < numTuples; j++)
        {
          if(m_Mask[j])
          {
            clusterDist[i][m_FeatureIds[j]] += KUtilities::GetDistance(m_InputData, (numCompDims * i), m_InputData, (numCompDims * j), numCompDims, m_DistMetric);
          }
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Mask[i])
      {
        for(usize j = 1; j < totalClusters; j++)
        {
          clusterDist[i][j] /= numTuplesPerFeature[j];
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Mask[i])
      {
        int32 cluster = m_FeatureIds[i];
        inClusterDist[i] = clusterDist[i][cluster];

        float64 minDist = std::numeric_limits<float64>::max();
        for(usize j = 1; j < totalClusters; j++)
        {
          if(cluster != j)
          {
            float64 dist = clusterDist[i][j];
            if(dist < minDist)
            {
              minDist = dist;
              outClusterMinDist[i] = dist;
            }
          }
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Mask[i])
      {
        m_OutputData[i] = (outClusterMinDist[i] - inClusterDist[i]) / (std::max(outClusterMinDist[i], inClusterDist[i]));
      }
    }
  }

private:
  using DataArrayT = DataArray<T>;
  const DataArrayT& m_InputData;
  Float64Array& m_OutputData;
  const Int32Array& m_FeatureIds;
  const BoolArray& m_Mask;
  usize m_NumClusters;
  KUtilities::DistanceMetric m_DistMetric;
};
} // namespace

// -----------------------------------------------------------------------------
Silhouette::Silhouette(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SilhouetteInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Silhouette::~Silhouette() noexcept = default;

// -----------------------------------------------------------------------------
void Silhouette::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& Silhouette::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> Silhouette::operator()()
{
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  std::unordered_set<int32> uniqueIds;

  usize numTuples = featureIds.getNumberOfTuples();
  for(usize i = 0; i < numTuples; i++)
  {
    uniqueIds.insert(featureIds[i]);
  }

  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  RunTemplateClass<SilhouetteTemplate, types::NoBooleanType>(clusteringArray.getDataType(), clusteringArray, m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SilhouetteArrayPath),
                                                             m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath), uniqueIds.size(), featureIds, m_InputValues->DistanceMetric);
  return {};
}
