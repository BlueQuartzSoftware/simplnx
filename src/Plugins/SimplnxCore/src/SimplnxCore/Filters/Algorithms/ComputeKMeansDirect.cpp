#include "ComputeKMeansDirect.hpp"

#include "ComputeKMeans.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <random>

using namespace nx::core;

namespace
{
/**
 * @class AllTrueMaskCompare
 * @brief Represents an all-selected mask without a synthetic array.
 *
 * The adapter reports the real tuple count. setValue() discards writes because
 * K-Means uses the mask only for selection.
 */
class AllTrueMaskCompare final : public MaskCompareUtilities::MaskCompare
{
public:
  explicit AllTrueMaskCompare(usize tupleCount)
  : m_TupleCount(tupleCount)
  {
  }

  bool bothTrue(usize, usize) const override
  {
    return true;
  }
  bool bothFalse(usize, usize) const override
  {
    return false;
  }
  bool isTrue(usize) const override
  {
    return true;
  }
  void setValue(usize, bool) override
  {
  }
  usize getNumberOfTuples() const override
  {
    return m_TupleCount;
  }
  usize getNumberOfComponents() const override
  {
    return 1;
  }
  usize countTrueValues() const override
  {
    return m_TupleCount;
  }

private:
  usize m_TupleCount = 0;
};

/**
 * @class ComputeKMeansTemplate
 * @brief Performs typed Lloyd iterations with direct element access.
 * @tparam T Input and centroid value type.
 */
template <typename T>
class ComputeKMeansTemplate
{
public:
  ComputeKMeansTemplate(ComputeKMeansDirect* filter, const IDataArray* inputIDataArray, IDataArray* meansIDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray,
                        usize numClusters, Int32AbstractDataStore& fIds, ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_InputArray(inputIDataArray->template getIDataStoreRefAs<AbstractDataStoreT>())
  , m_Means(meansIDataArray->template getIDataStoreRefAs<AbstractDataStoreT>())
  , m_Mask(maskDataArray)
  , m_NumClusters(numClusters)
  , m_FeatureIds(fIds)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  {
  }
  ~ComputeKMeansTemplate() = default;

  ComputeKMeansTemplate(const ComputeKMeansTemplate&) = delete;
  void operator=(const ComputeKMeansTemplate&) = delete;

  // -----------------------------------------------------------------------------
  /**
   * @brief Initializes centroids and runs assignment and mean phases.
   *
   * Centroid sampling permits duplicates. For multi-tuple input, the legacy
   * index formula excludes the final tuple. The convergence test reads flat
   * means indices 1 through K and does not inspect all components.
   *
   * Cancellation can stop an inner phase after it changes part of an output.
   */
  void operator()()
  {
    usize numTuples = m_InputArray.getNumberOfTuples();
    int32 numCompDims = m_InputArray.getNumberOfComponents();

    const usize rangeMax = numTuples - 1;

    std::mt19937_64 gen(m_Seed);
    std::uniform_real_distribution<float64> dist(0.0, 1.0);

    std::vector<usize> clusterIdxs(m_NumClusters);

    usize clusterChoices = 0;
    while(clusterChoices < m_NumClusters)
    {
      usize index = std::floor(dist(gen) * static_cast<float64>(rangeMax));
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
  using AbstractDataStoreT = AbstractDataStore<T>;
  ComputeKMeansDirect* m_Filter;
  const AbstractDataStoreT& m_InputArray;
  AbstractDataStoreT& m_Means;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  usize m_NumClusters;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
  /**
   * @brief Tests whether two values differ by less than epsilon.
   * @tparam K Compared value type.
   * @param a First value.
   * @param b Second value.
   * @param epsilon Exclusive difference limit.
   * @return True when the absolute difference is less than epsilon.
   */
  template <typename K>
  bool closeEnough(const K& a, const K& b, const K& epsilon = std::numeric_limits<K>::epsilon())
  {
    return (epsilon > fabs(a - b));
  }

  // -----------------------------------------------------------------------------
  /**
   * @brief Assigns each selected tuple to its nearest centroid.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   *
   * Direct element access is efficient for resident data. Cancellation leaves
   * earlier assignments in FeatureIds.
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
          float64 dist = ClusterUtilities::GetDistance(m_InputArray, (dims * i), m_Means, (dims * (j + 1)), dims, m_DistMetric);
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
   * @brief Recomputes arithmetic means with direct element access.
   * @param tuples Number of input tuples.
   * @param dims Number of components in each tuple.
   *
   * All tuples contribute to their current assignment. Masked tuples normally
   * remain in reserved bucket zero. The function rescans input once for each
   * component to preserve the original direct DataStore accumulation path.
   * Cancellation can leave a partially recomputed means array.
   */
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
ComputeKMeansDirect::ComputeKMeansDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeKMeansInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKMeansDirect::~ComputeKMeansDirect() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMeansDirect::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMeansDirect::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeKMeansDirect::operator()()
{
  auto* clusteringArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->ClusteringArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::exception&)
    {
      return MakeErrorResult(-54060, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }
  else
  {
    maskCompare = std::make_unique<AllTrueMaskCompare>(clusteringArray->getNumberOfTuples());
  }

  if(maskCompare->countTrueValues() == 0)
  {
    return MakeErrorResult(-54063, "Compute K Means cannot initialize clusters because the mask contains no selected tuples.");
  }

  RunTemplateClass<ComputeKMeansTemplate, types::NoBooleanType>(clusteringArray->getDataType(), this, clusteringArray, m_DataStructure.getDataAs<IDataArray>(m_InputValues->MeansArrayPath),
                                                                maskCompare, m_InputValues->InitClusters, m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef(),
                                                                m_InputValues->DistanceMetric, m_InputValues->Seed);

  return {};
}
