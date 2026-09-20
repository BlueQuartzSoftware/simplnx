#include "SilhouetteDirect.hpp"

#include "Silhouette.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @class SilhouetteTemplate
 * @brief Computes typed silhouette scores with a complete distance table.
 * @tparam T Specifies the clustering-array value type.
 *
 * Retaining accumulated distance for each tuple and cluster avoids rereading
 * input pairs. This trades N by (K + 1) resident memory for direct reuse.
 */
template <typename T>
class SilhouetteTemplate
{
public:
  /**
   * @brief Initializes the typed resident calculation.
   * @param inputIDataArray Supplies clustering tuples.
   * @param outputDataArray Receives silhouette scores.
   * @param maskDataArray Supplies an optional mask comparator.
   * @param useMask True to apply maskDataArray.
   * @param numClusters Number of distinct Feature IDs.
   * @param featureIds Supplies one cluster ID per tuple.
   * @param distMetric Selects the tuple distance function.
   * @pre All arguments outlive this calculation.
   */
  SilhouetteTemplate(const IDataArray& inputIDataArray, Float64AbstractDataStore& outputDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray, bool useMask,
                     usize numClusters, const Int32AbstractDataStore& featureIds, ClusterUtilities::DistanceMetric distMetric)
  : m_InputData(inputIDataArray.template getIDataStoreRefAs<AbstractDataStoreT>())
  , m_OutputData(outputDataArray)
  , m_FeatureIds(featureIds)
  , m_Mask(maskDataArray)
  , m_UseMask(useMask)
  , m_NumClusters(numClusters)
  , m_DistMetric(distMetric)
  {
  }

  /**
   * @brief Builds the distance table and writes each score.
   *
   * The own-cluster mean includes self-distance. Empty cluster columns divide by
   * zero but cannot win a finite minimum. A zero score denominator produces NaN.
   */
  void operator()()
  {
    const usize numTuples = m_InputData.getNumberOfTuples();
    const usize numCompDims = m_InputData.getNumberOfComponents();
    const usize totalClusters = m_NumClusters + 1;
    std::vector<float64> inClusterDist(numTuples, 0.0);
    std::vector<float64> outClusterMinDist(numTuples, 0.0);
    std::vector<float64> numTuplesPerFeature(totalClusters, 0.0);
    std::vector<std::vector<float64>> clusterDist(numTuples, std::vector<float64>(totalClusters, 0.0));

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        numTuplesPerFeature[m_FeatureIds[i]]++;
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        for(usize j = 0; j < numTuples; j++)
        {
          if(!m_UseMask || m_Mask->isTrue(j))
          {
            clusterDist[i][m_FeatureIds[j]] += ClusterUtilities::GetDistance(m_InputData, numCompDims * i, m_InputData, numCompDims * j, numCompDims, m_DistMetric);
          }
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        for(usize j = 1; j < totalClusters; j++)
        {
          clusterDist[i][j] /= numTuplesPerFeature[j];
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        const int32 cluster = m_FeatureIds[i];
        inClusterDist[i] = clusterDist[i][cluster];

        float64 minDist = std::numeric_limits<float64>::max();
        for(usize j = 1; j < totalClusters; j++)
        {
          if(cluster != j)
          {
            const float64 dist = clusterDist[i][j];
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
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        m_OutputData[i] = (outClusterMinDist[i] - inClusterDist[i]) / std::max(outClusterMinDist[i], inClusterDist[i]);
      }
      else
      {
        m_OutputData[i] = 0.0;
      }
    }
  }

private:
  using AbstractDataStoreT = AbstractDataStore<T>;
  const AbstractDataStoreT& m_InputData;
  Float64AbstractDataStore& m_OutputData;
  const Int32AbstractDataStore& m_FeatureIds;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  bool m_UseMask = false;
  usize m_NumClusters;
  ClusterUtilities::DistanceMetric m_DistMetric;
};
} // namespace

SilhouetteDirect::SilhouetteDirect(DataStructure& dataStructure, const IFilter::MessageHandler&, const std::atomic_bool&, const SilhouetteInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
{
}

SilhouetteDirect::~SilhouetteDirect() noexcept = default;

Result<> SilhouetteDirect::operator()()
{
  // Distinct-ID count sizes the direct distance table. IDs still index it directly.
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  std::unordered_set<int32> uniqueIds;
  for(usize i = 0; i < featureIds.getNumberOfTuples(); i++)
  {
    uniqueIds.insert(featureIds[i]);
  }

  // Avoid a cell-sized synthetic all-true mask when masking is disabled.
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range&)
    {
      return MakeErrorResult(-54080, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  const auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& outputStore = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SilhouetteArrayPath).getDataStoreRef();
  RunTemplateClass<SilhouetteTemplate, types::NoBooleanType>(clusteringArray.getDataType(), clusteringArray, outputStore, maskCompare, m_InputValues->UseMask, uniqueIds.size(), featureIds,
                                                             m_InputValues->DistanceMetric);
  return {};
}
