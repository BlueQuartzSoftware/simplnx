#include "ComputeKMedoidsScanline.hpp"

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
template <typename T>
class KMedoidsTemplate
{
public:
  KMedoidsTemplate(ComputeKMedoidsScanline* filter, const IDataArray* inputIDataArray, IDataArray* medoidsIDataArray, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskDataArray,
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

    // OOC: use bulk I/O to initialize medoids
    auto tupleBuf = std::make_unique<T[]>(numCompDims);
    for(usize i = 0; i < m_NumClusters; i++)
    {
      m_InputArray.copyIntoBuffer(numCompDims * clusterIdxs[i], nonstd::span<T>(tupleBuf.get(), numCompDims));
      m_Medoids.copyFromBuffer(numCompDims * (i + 1), nonstd::span<const T>(tupleBuf.get(), numCompDims));
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
  ComputeKMedoidsScanline* m_Filter;
  const AbstractDataStoreT& m_InputArray;
  AbstractDataStoreT& m_Medoids;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  usize m_NumClusters = 0;
  Int32AbstractDataStore& m_FeatureIds;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;

  // -----------------------------------------------------------------------------
  // OOC: cache medoids locally, process input and featureIds in chunks
  void findClusters(usize tuples, int32 dims)
  {
    // Cache medoids (small: numClusters * dims)
    const usize medoidsSize = (m_NumClusters + 1) * dims;
    std::vector<T> medoidsCache(medoidsSize);
    m_Medoids.copyIntoBuffer(0, nonstd::span<T>(medoidsCache.data(), medoidsSize));

    constexpr usize k_ChunkTuples = 65536;
    auto inputBuf = std::make_unique<T[]>(k_ChunkTuples * dims);
    std::vector<int32> fidsBuf(k_ChunkTuples);

    for(usize startTup = 0; startTup < tuples; startTup += k_ChunkTuples)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      const usize endTup = std::min(startTup + k_ChunkTuples, tuples);
      const usize count = endTup - startTup;

      m_InputArray.copyIntoBuffer(startTup * dims, nonstd::span<T>(inputBuf.get(), count * dims));
      m_FeatureIds.copyIntoBuffer(startTup, nonstd::span<int32>(fidsBuf.data(), count));

      for(usize local = 0; local < count; local++)
      {
        if(m_Mask->isTrue(startTup + local))
        {
          float64 minDist = std::numeric_limits<float64>::max();
          for(int32 j = 0; j < m_NumClusters; j++)
          {
            float64 dist = ClusterUtilities::GetDistance(inputBuf.get(), (dims * local), medoidsCache, (dims * (j + 1)), dims, m_DistMetric);
            if(dist < minDist)
            {
              minDist = dist;
              fidsBuf[local] = j + 1;
            }
          }
        }
      }

      m_FeatureIds.copyFromBuffer(startTup, nonstd::span<const int32>(fidsBuf.data(), count));
    }
  }

  // -----------------------------------------------------------------------------
  // OOC: process one cluster at a time to avoid O(n) total member list allocation.
  // Peak memory is O(max_cluster_size), not O(n).
  std::vector<float64> optimizeClusters(usize tuples, int32 dims, std::vector<usize>& clusterIdxs)
  {
    std::vector<float64> minCosts(m_NumClusters, std::numeric_limits<float64>::max());

    constexpr usize k_ChunkSize = 65536;
    std::vector<int32> fidsBuf(k_ChunkSize);
    auto tupleBufJ = std::make_unique<T[]>(dims);
    auto tupleBufK = std::make_unique<T[]>(dims);

    // Process one cluster at a time — build member list, compute costs, then release
    for(usize i = 0; i < m_NumClusters; i++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      // Scan featureIds in chunks to find members of cluster i only
      std::vector<usize> members;
      for(usize start = 0; start < tuples; start += k_ChunkSize)
      {
        usize count = std::min(k_ChunkSize, tuples - start);
        m_FeatureIds.copyIntoBuffer(start, nonstd::span<int32>(fidsBuf.data(), count));
        for(usize local = 0; local < count; local++)
        {
          if(fidsBuf[local] == static_cast<int32>(i + 1) && m_Mask->isTrue(start + local))
          {
            members.push_back(start + local);
          }
        }
      }

      // Find the member that minimizes total intra-cluster distance
      for(usize mj = 0; mj < members.size(); mj++)
      {
        if(m_Filter->getCancel())
        {
          return {};
        }
        usize j = members[mj];
        m_InputArray.copyIntoBuffer(j * dims, nonstd::span<T>(tupleBufJ.get(), dims));

        float64 cost = 0.0;
        for(usize mk = 0; mk < members.size(); mk++)
        {
          if(m_Filter->getCancel())
          {
            return {};
          }
          usize k = members[mk];
          m_InputArray.copyIntoBuffer(k * dims, nonstd::span<T>(tupleBufK.get(), dims));
          cost += ClusterUtilities::GetDistance(tupleBufJ.get(), 0, tupleBufK.get(), 0, dims, m_DistMetric);
        }

        if(cost < minCosts[i])
        {
          minCosts[i] = cost;
          clusterIdxs[i] = j;
        }
      }
      // members is released here at end of loop iteration
    }

    // Update medoids from best candidates
    for(usize i = 0; i < m_NumClusters; i++)
    {
      m_InputArray.copyIntoBuffer(dims * clusterIdxs[i], nonstd::span<T>(tupleBufJ.get(), dims));
      m_Medoids.copyFromBuffer(dims * (i + 1), nonstd::span<const T>(tupleBufJ.get(), dims));
    }

    return minCosts;
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeKMedoidsScanline::ComputeKMedoidsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKMedoidsScanline::~ComputeKMedoidsScanline() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeKMedoidsScanline::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeKMedoidsScanline::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeKMedoidsScanline::operator()()
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
