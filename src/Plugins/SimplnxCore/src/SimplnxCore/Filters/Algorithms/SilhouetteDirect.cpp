#include "SilhouetteDirect.hpp"

#include "Silhouette.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
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
   * @param numClusters Number of distinct positive Cluster Ids.
   * @param featureIds Supplies one dense cluster index per tuple. Index zero represents Cluster Id zero.
   * @param distMetric Selects the tuple distance function.
   * @pre All arguments outlive this calculation.
   */
  SilhouetteTemplate(const IDataArray& inputIDataArray, Float64AbstractDataStoreType& outputDataArray, const std::unique_ptr<mask_compare_utilities::MaskCompare>& maskDataArray, bool useMask,
                     usize numClusters, const std::vector<usize>& featureIds, cluster_utilities::DistanceMetric distMetric)
  : m_InputData(inputIDataArray.template getIDataStoreRefAs<AbstractDataStoreTType>())
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
   * The own-cluster mean includes self-distance. Empty clusters cannot supply the competing mean.
   * Cluster zero receives scores but cannot supply the competing mean. A zero score denominator produces NaN.
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
            clusterDist[i][m_FeatureIds[j]] += cluster_utilities::GetDistance(m_InputData, numCompDims * i, m_InputData, numCompDims * j, numCompDims, m_DistMetric);
          }
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        for(usize j = 0; j < totalClusters; j++)
        {
          if(numTuplesPerFeature[j] > 0.0)
          {
            clusterDist[i][j] /= numTuplesPerFeature[j];
          }
        }
      }
    }

    for(usize i = 0; i < numTuples; i++)
    {
      if(!m_UseMask || m_Mask->isTrue(i))
      {
        const usize cluster = m_FeatureIds[i];
        inClusterDist[i] = clusterDist[i][cluster];

        float64 minDist = std::numeric_limits<float64>::max();
        for(usize j = 1; j < totalClusters; j++)
        {
          if(cluster != j && numTuplesPerFeature[j] > 0.0)
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
  using AbstractDataStoreTType = AbstractDataStore<T>;
  const AbstractDataStoreTType& m_InputData;
  Float64AbstractDataStoreType& m_OutputData;
  const std::vector<usize>& m_FeatureIds;
  const std::unique_ptr<mask_compare_utilities::MaskCompare>& m_Mask;
  bool m_UseMask = false;
  usize m_NumClusters;
  cluster_utilities::DistanceMetric m_DistMetric;
};
} // namespace

SilhouetteDirect::SilhouetteDirect(DataStructure& dataStructure, const IFilter::MessageHandler& /*unused*/, const std::atomic_bool& /*unused*/, const SilhouetteInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
{
}

Result<> SilhouetteDirect::operator()()
{
  // Dense indices keep sparse Cluster Ids within the distance table. Cluster zero retains index zero.
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->feature_ids_array_path).getDataStoreRef();
  std::unordered_map<int32, usize> clusterIndices;
  std::vector<usize> denseFeatureIds(featureIds.getNumberOfTuples(), 0);
  for(usize tupleIdx = 0; tupleIdx < featureIds.getNumberOfTuples(); tupleIdx++)
  {
    const int32 clusterId = featureIds[tupleIdx];
    if(clusterId < 0)
    {
      return MakeErrorResult(-54081, fmt::format("Cluster Ids DataArray '{}' contains negative value {} at tuple {}. Cluster Ids must be nonnegative.",
                                                 m_InputValues->feature_ids_array_path.toString(), clusterId, tupleIdx));
    }
    if(clusterId > 0)
    {
      const auto [iter, inserted] = clusterIndices.try_emplace(clusterId, clusterIndices.size() + 1);
      denseFeatureIds[tupleIdx] = iter->second;
    }
  }

  // Avoid a cell-sized synthetic all-true mask when masking is disabled.
  std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare;
  if(m_InputValues->use_mask)
  {
    try
    {
      maskCompare = mask_compare_utilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->mask_array_path);
    } catch(const std::out_of_range&)
    {
      return MakeErrorResult(-54080, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->mask_array_path.toString()));
    }
  }

  const auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->clustering_array_path);
  auto& outputStore = m_DataStructure.getDataRefAs<Float64ArrayType>(m_InputValues->silhouette_array_path).getDataStoreRef();
  RunTemplateClass<SilhouetteTemplate, types::NoBooleanType>(clusteringArray.getDataType(), clusteringArray, outputStore, maskCompare, m_InputValues->use_mask, clusterIndices.size(), denseFeatureIds,
                                                             m_InputValues->distance_metric);
  return {};
}
