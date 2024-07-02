#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
template <typename T>
class FindEpsilonNeighborhoodsImpl
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  FindEpsilonNeighborhoodsImpl(DBSCAN* filter, float64 epsilon, const AbstractDataStoreT& inputData, const std::unique_ptr<MaskCompare>& mask, usize numCompDims, usize numTuples,
                               ClusterUtilities::DistanceMetric distMetric, std::vector<std::list<usize>>& neighborhoods)
  : m_Filter(filter)
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputData)
  , m_Mask(mask)
  , m_NumCompDims(numCompDims)
  , m_NumTuples(numTuples)
  , m_DistMetric(distMetric)
  , m_Neighborhoods(neighborhoods)
  {
  }

  void compute(usize start, usize end) const
  {
    for(usize i = start; i < end; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }
      if(m_Mask->isTrue(i))
      {
        // directly  to try to convince compiler to construct in place
        m_Neighborhoods[i] = epsilon_neighbors(i);
      }
    }
  }

  [[nodiscard]] std::list<usize> epsilon_neighbors(usize index) const
  {
    std::list<usize> neighbors;

    for(usize i = 0; i < m_NumTuples; i++)
    {
      if(m_Mask->isTrue(i))
      {
        float64 dist = ClusterUtilities::GetDistance(m_InputDataStore, (m_NumCompDims * index), m_InputDataStore, (m_NumCompDims * i), m_NumCompDims, m_DistMetric);
        if(dist < m_Epsilon)
        {
          neighbors.push_back(i);
        }
      }
    }

    return neighbors;
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  DBSCAN* m_Filter;
  float64 m_Epsilon;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompare>& m_Mask;
  usize m_NumCompDims;
  usize m_NumTuples;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::vector<std::list<usize>>& m_Neighborhoods;
};

template <typename T, bool PrecacheV = true, bool RandomInitV = true>
class DBSCANTemplate
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  DBSCANTemplate(DBSCAN* filter, const AbstractDataStoreT& inputDataStore, const std::unique_ptr<MaskCompare>& maskDataArray, AbstractDataStore<int32>& fIdsDataStore, float32 epsilon, int32 minPoints,
                 ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_InputDataStore(inputDataStore)
  , m_Mask(maskDataArray)
  , m_FeatureIds(fIdsDataStore)
  , m_Epsilon(epsilon)
  , m_MinPoints(minPoints)
  , m_DistMetric(distMetric)
  , m_Seed(seed)
  {
  }
  ~DBSCANTemplate() = default;

  DBSCANTemplate(const DBSCANTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const DBSCANTemplate&) = delete; // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputDataStore.getNumberOfTuples();
    usize numCompDims = m_InputDataStore.getNumberOfComponents();
    std::vector<bool> visited(numTuples, false);   // Uses one bit per value for space efficiency
    std::vector<bool> clustered(numTuples, false); // Uses one bit per value for space efficiency

    auto minDist = static_cast<float64>(m_Epsilon);
    int32 cluster = 0;

    std::vector<std::list<usize>> epsilonNeighborhoods;

    if constexpr(PrecacheV)
    {
      // In-memory only with current implementation for speed with std::list
      epsilonNeighborhoods = std::vector<std::list<usize>>(numTuples);

      m_Filter->updateProgress("Finding Neighborhoods in parallel...");
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0ULL, numTuples);
      dataAlg.execute(FindEpsilonNeighborhoodsImpl<T>(m_Filter, minDist, m_InputDataStore, m_Mask, numCompDims, numTuples, m_DistMetric, epsilonNeighborhoods));

      m_Filter->updateProgress("Neighborhoods found.");
    }

    std::mt19937_64 gen(m_Seed);
    std::uniform_int_distribution<usize> dist(0, numTuples - 1);

    m_Filter->updateProgress("Beginning clustering...");
    auto start = std::chrono::steady_clock::now();
    usize i = 0;
    uint8 misses = 0;
    while(std::find(visited.begin(), visited.end(), false) != visited.end())
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      usize index;
      if constexpr(!RandomInitV)
      {
        index = i;
        if(i >= numTuples)
        {
          break;
        }
        i++;
      }
      if constexpr(RandomInitV)
      {
        index = dist(gen);
      }

      if(visited[index])
      {
        if(misses >= 10)
        {
          auto findIter = std::find(visited.begin(), visited.end(), false);
          if(findIter == visited.end())
          {
            break;
          }
          index = std::distance(visited.begin(), findIter);

          if constexpr(RandomInitV)
          {
            dist = std::uniform_int_distribution<usize>(index, numTuples - 1);
          }
        }
        else
        {
          misses++;
          continue;
        }
      }

      misses = 0;

      if(m_Mask->isTrue(index))
      {
        visited[index] = true;
        auto now = std::chrono::steady_clock::now();
        // Only send updates every 1 second
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          float32 progress = (static_cast<float32>(index) / static_cast<float32>(numTuples)) * 100.0f;
          m_Filter->updateProgress(fmt::format("Scanning Data || Visited Point {} of {} || {:.2f}% Completed", index, numTuples, progress));
          start = std::chrono::steady_clock::now();
        }

        std::list<usize> neighbors;
        if constexpr(PrecacheV)
        {
          neighbors = epsilonNeighborhoods[index];
        }
        if constexpr(!PrecacheV)
        {
          for(usize j = 0; j < numTuples; j++)
          {
            if(m_Mask->isTrue(j))
            {
              float64 distance = ClusterUtilities::GetDistance(m_InputDataStore, (numCompDims * index), m_InputDataStore, (numCompDims * j), numCompDims, m_DistMetric);
              if(distance < m_Epsilon)
              {
                neighbors.push_back(j);
              }
            }
          }
        }

        if(static_cast<int32>(neighbors.size()) < m_MinPoints)
        {
          m_FeatureIds[index] = 0;
          clustered[index] = true;
        }
        else
        {
          if(m_Filter->getCancel())
          {
            return;
          }
          cluster++;
          m_FeatureIds[index] = cluster;
          clustered[index] = true;

          for(auto&& idx : neighbors)
          {
            if(m_Mask->isTrue(idx))
            {
              if(!visited[idx])
              {
                visited[idx] = true;

                std::list<usize> neighbors_prime;
                if constexpr(PrecacheV)
                {
                  neighbors_prime = epsilonNeighborhoods[idx];
                }
                if constexpr(!PrecacheV)
                {
                  for(usize j = 0; j < numTuples; j++)
                  {
                    if(m_Mask->isTrue(j))
                    {
                      float64 distance = ClusterUtilities::GetDistance(m_InputDataStore, (numCompDims * idx), m_InputDataStore, (numCompDims * j), numCompDims, m_DistMetric);
                      if(distance < m_Epsilon)
                      {
                        neighbors_prime.push_back(j);
                      }
                    }
                  }
                }

                if(static_cast<int32>(neighbors_prime.size()) >= m_MinPoints)
                {
                  neighbors.splice(std::end(neighbors), neighbors_prime);
                }
              }
              if(!clustered[idx])
              {
                m_FeatureIds[idx] = cluster;
                clustered[idx] = true;
              }
            }
          }
        }
      }
      else
      {
        visited[index] = true;
      }
    }
    m_Filter->updateProgress("Clustering Complete!");
  }

private:
  DBSCAN* m_Filter;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompare>& m_Mask;
  AbstractDataStore<int32>& m_FeatureIds;
  float32 m_Epsilon;
  int32 m_MinPoints;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;
};

struct DBSCANFunctor
{
  template <typename T>
  void operator()(bool cache, bool useRandom, DBSCAN* filter, const IDataArray& inputIDataArray, const std::unique_ptr<MaskCompare>& maskCompare, Int32Array& fIds, float32 epsilon, int32 minPoints,
                  ClusterUtilities::DistanceMetric distMetric, std::mt19937_64::result_type seed)
  {
    if(cache)
    {
      if(useRandom)
      {
        DBSCANTemplate<T, true, true>(filter, dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed)();
      }
      else
      {
        DBSCANTemplate<T, true, false>(filter, dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed)();
      }
    }
    else
    {
      if(useRandom)
      {
        DBSCANTemplate<T, false, true>(filter, dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed)();
      }
      else
      {
        DBSCANTemplate<T, false, false>(filter, dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef(), maskCompare, fIds.getDataStoreRef(), epsilon, minPoints, distMetric, seed)();
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

// -----------------------------------------------------------------------------
void DBSCAN::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& DBSCAN::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DBSCAN::operator()()
{
  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54060, message);
  }

  ExecuteNeighborFunction(DBSCANFunctor{}, clusteringArray.getDataType(), m_InputValues->AllowCaching, m_InputValues->UseRandom, this, clusteringArray, maskCompare, featureIds, m_InputValues->Epsilon,
                          m_InputValues->MinPoints, m_InputValues->DistanceMetric, m_InputValues->Seed);

  updateProgress("Resizing Clustering Attribute Matrix...");
  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(AttributeMatrix::ShapeType{static_cast<usize>(maxCluster + 1)});

  return {};
}
